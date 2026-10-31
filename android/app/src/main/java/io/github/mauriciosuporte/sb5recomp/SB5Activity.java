package io.github.mauriciosuporte.sb5recomp;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInstaller;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/**
 * SDLActivity plus the file import the shared launcher (recomp-ui) asks for.
 *
 * Android apps cannot browse shared storage by path, so the launcher's
 * "Browse For ROM" calls {@link #recompPickFile} over JNI. It opens the
 * system document picker, copies the chosen file into the game folder's
 * roms/ (the app's external files dir, where the game also looks for ROMs by
 * SHA-256) and returns that path. Nothing is uploaded or shared: the copy
 * stays in the app's own storage.
 *
 * It also gives the native game what Android has no command-line tools for:
 * HTTPS requests ({@link #sb5Http}: Boss Rush ranking, update check) and the
 * in-place update ({@link #sb5InstallApk}: download the release APK and hand
 * it to the system installer, which keeps the app's files).
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

    // ---- HTTPS for the native side ----------------------------------------

    private static final String USER_AGENT =
            "DiscordBot (https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp, 1)";

    private static void setMethod(HttpURLConnection c, String method) throws Exception {
        try {
            c.setRequestMethod(method);
        } catch (ProtocolException e) {
            // Some stacks reject PATCH (Discord needs it to edit the board).
            Field f = HttpURLConnection.class.getDeclaredField("method");
            f.setAccessible(true);
            f.set(c, method);
        }
    }

    /**
     * Runs one request on the calling (native) thread. Returns
     * "&lt;status&gt;\n&lt;body&gt;"; status 0 when the request could not be made.
     */
    public String sb5Http(String method, String url, String body) {
        HttpURLConnection c = null;
        try {
            c = (HttpURLConnection) new URL(url).openConnection();
            c.setConnectTimeout(8000);
            c.setReadTimeout(15000);
            c.setInstanceFollowRedirects(true);
            setMethod(c, method);
            c.setRequestProperty("User-Agent", USER_AGENT);
            c.setRequestProperty("Accept", "application/json");
            if (body != null) {
                byte[] data = body.getBytes(StandardCharsets.UTF_8);
                c.setDoOutput(true);
                c.setRequestProperty("Content-Type", "application/json");
                c.setFixedLengthStreamingMode(data.length);
                try (OutputStream os = c.getOutputStream()) { os.write(data); }
            }
            int status = c.getResponseCode();
            InputStream in = status >= 400 ? c.getErrorStream() : c.getInputStream();
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            if (in != null) {
                try (InputStream s = in) {
                    byte[] buf = new byte[16384];
                    int n;
                    while ((n = s.read(buf)) > 0 && out.size() < (1 << 20)) out.write(buf, 0, n);
                }
            }
            return status + "\n" + out.toString("UTF-8");
        } catch (Exception e) {
            Log.w(TAG, "http " + method + " failed: " + e);
            return "0\n";
        } finally {
            if (c != null) c.disconnect();
        }
    }

    // ---- in-place update ---------------------------------------------------

    private static final String ACTION_INSTALL_STATUS =
            "io.github.mauriciosuporte.sb5recomp.INSTALL_STATUS";
    private volatile String updateStatus = "";

    /** Last update progress line, for the launcher note. */
    public String sb5UpdateStatus() { return updateStatus; }

    /**
     * Starts downloading the release APK at {@code url} and installing it
     * over this app (same package and signing key, so saves, ROMs and
     * settings in the app's folders are kept). Returns a line for the
     * launcher; the system shows its own "Update this app?" confirmation.
     */
    public String sb5InstallApk(final String url) {
        if (Build.VERSION.SDK_INT >= 26 && !getPackageManager().canRequestPackageInstalls()) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    try {
                        startActivity(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                                Uri.parse("package:" + getPackageName())));
                    } catch (Exception e) {
                        Log.e(TAG, "cannot open install permission settings", e);
                    }
                }
            });
            return "Allow \"Install unknown apps\" for Super Bomberman 5, come back and press Update again. "
                    + "/ Permita \"Instalar apps desconhecidos\", volte e toque em Atualizar de novo.";
        }
        updateStatus = "Downloading the update... / Baixando a atualização...";
        new Thread(new Runnable() {
            @Override
            public void run() { downloadAndInstall(url); }
        }, "sb5-update").start();
        return updateStatus;
    }

    private void downloadAndInstall(String url) {
        HttpURLConnection c = null;
        PackageInstaller.Session session = null;
        try {
            c = (HttpURLConnection) new URL(url).openConnection();
            c.setConnectTimeout(10000);
            c.setReadTimeout(30000);
            c.setInstanceFollowRedirects(true);
            c.setRequestProperty("User-Agent", USER_AGENT);
            int status = c.getResponseCode();
            if (status != 200) throw new Exception("HTTP " + status);
            long size = c.getContentLengthLong();
            PackageInstaller installer = getPackageManager().getPackageInstaller();
            PackageInstaller.SessionParams params =
                    new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
            params.setAppPackageName(getPackageName());
            if (size > 0) params.setSize(size);
            int id = installer.createSession(params);
            session = installer.openSession(id);
            long done = 0;
            try (InputStream in = c.getInputStream();
                 OutputStream os = session.openWrite("sb5-update.apk", 0, size > 0 ? size : -1)) {
                byte[] buf = new byte[1 << 16];
                int n;
                while ((n = in.read(buf)) > 0) {
                    os.write(buf, 0, n);
                    done += n;
                    if (size > 0)
                        updateStatus = "Downloading the update... " + (done * 100 / size) + "% / Baixando a atualização...";
                }
                session.fsync(os);
            }
            Intent cb = new Intent(this, SB5Activity.class).setAction(ACTION_INSTALL_STATUS)
                    .addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            int flags = PendingIntent.FLAG_UPDATE_CURRENT;
            if (Build.VERSION.SDK_INT >= 31) flags |= 0x02000000; // FLAG_MUTABLE: the installer adds extras
            PendingIntent pending = PendingIntent.getActivity(this, id, cb, flags);
            updateStatus = "Installing: confirm on the system screen. / Instalando: confirme na tela do sistema.";
            session.commit(pending.getIntentSender());
            session.close();
            session = null;
            Log.i(TAG, "update downloaded (" + done + " bytes), install session committed");
        } catch (Exception e) {
            Log.e(TAG, "update failed", e);
            updateStatus = "Update failed: " + e.getMessage() + " / Falha ao atualizar.";
            if (session != null) session.abandon();
        } finally {
            if (c != null) c.disconnect();
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (intent == null || !ACTION_INSTALL_STATUS.equals(intent.getAction())) return;
        int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
        if (status == PackageInstaller.STATUS_PENDING_USER_ACTION) {
            Intent confirm = intent.getParcelableExtra(Intent.EXTRA_INTENT);
            if (confirm != null) startActivity(confirm);
        } else if (status != PackageInstaller.STATUS_SUCCESS) {
            String m = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
            Log.e(TAG, "install status " + status + ": " + m);
            updateStatus = "Install failed (" + (m != null ? m : status) + "). If the app was installed from a "
                    + "test build, uninstall it once and install the new APK. / Instalação falhou.";
        }
    }
}
