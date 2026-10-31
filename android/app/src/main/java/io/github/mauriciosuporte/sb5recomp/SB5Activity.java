package io.github.mauriciosuporte.sb5recomp;

import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * SDLActivity plus the file import the shared launcher (recomp-ui) asks for.
 *
 * Android apps cannot browse shared storage by path, so the launcher's
 * "Browse For ROM" calls {@link #recompPickFile} over JNI. It opens the
 * system document picker, copies the chosen file into the game folder's
 * roms/ (the app's external files dir, where the game also looks for ROMs by
 * SHA-256) and returns that path. Nothing is uploaded or shared: the copy
 * stays in the app's own storage.
 */
public class SB5Activity extends SDLActivity {
    private static final String TAG = "SB5";
    private static final int REQUEST_PICK = 0x5B5;

    private final Object pickLock = new Object();
    private boolean pickDone;
    private String pickResult;

    /** Called from the native launcher thread; blocks until the picker closes. */
    public String recompPickFile(final String title) {
        synchronized (pickLock) {
            pickDone = false;
            pickResult = null;
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                try {
                    startActivityForResult(Intent.createChooser(intent, title), REQUEST_PICK);
                } catch (Exception e) {
                    Log.e(TAG, "document picker unavailable", e);
                    finishPick(null);
                }
            }
        });
        synchronized (pickLock) {
            while (!pickDone) {
                try {
                    pickLock.wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    return null;
                }
            }
            return pickResult;
        }
    }

    private void finishPick(String path) {
        synchronized (pickLock) {
            pickResult = path;
            pickDone = true;
            pickLock.notifyAll();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_PICK) return;
        final Uri uri = (resultCode == RESULT_OK && data != null) ? data.getData() : null;
        if (uri == null) {
            finishPick(null);
            return;
        }
        // Copy off the UI thread: a ROM is a few MiB.
        new Thread(new Runnable() {
            @Override
            public void run() {
                finishPick(importDocument(uri));
            }
        }, "sb5-import").start();
    }

    private String importDocument(Uri uri) {
        File root = getExternalFilesDir(null);
        if (root == null) root = getFilesDir();
        File dir = new File(root, "roms");
        if (!dir.isDirectory() && !dir.mkdirs()) {
            Log.e(TAG, "cannot create " + dir);
            return null;
        }
        File out = new File(dir, safeName(displayName(uri)));
        try (InputStream in = getContentResolver().openInputStream(uri);
             OutputStream os = new FileOutputStream(out)) {
            if (in == null) return null;
            byte[] buf = new byte[1 << 16];
            int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
        } catch (Exception e) {
            Log.e(TAG, "import failed", e);
            out.delete();
            return null;
        }
        Log.i(TAG, "imported " + out.getAbsolutePath() + " (" + out.length() + " bytes)");
        return out.getAbsolutePath();
    }

    private String displayName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri,
                new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                String name = c.getString(0);
                if (name != null && !name.isEmpty()) return name;
            }
        } catch (Exception e) {
            Log.w(TAG, "no display name", e);
        }
        String last = uri.getLastPathSegment();
        return last != null ? last : "imported.sfc";
    }

    private static String safeName(String name) {
        String s = name.replace('/', '_').replace('\\', '_').trim();
        if (s.isEmpty() || s.equals(".") || s.equals("..")) s = "imported.sfc";
        return s;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }
}
