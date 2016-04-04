package com.cnk.activities;

import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.cnk.R;
import com.cnk.communication.NetworkHandler;
import com.cnk.data.exhibits.ExhibitsData;
import com.cnk.data.experiment.ExperimentData;
import com.cnk.data.experiment.survey.Survey;
import com.cnk.data.map.MapData;
import com.cnk.data.raports.ReadyRaports;
import com.cnk.database.DatabaseHelper;
import com.cnk.exceptions.DatabaseLoadException;
import com.cnk.notificators.Observer;
import com.cnk.ui.Utils;

import java.util.ArrayList;

public class StartScreen extends AppCompatActivity implements Observer {
    private static final String LOG_TAG = "StartScreen";
    private static boolean dataLoaded;
    private DatabaseHelper dbHelper;
    private ProgressDialog progressBar;
    private Thread progressDialogUpdate;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dbHelper = new DatabaseHelper(this.getApplicationContext());
        setContentView(R.layout.activity_start_screen);
        ExperimentData.getInstance().setDbHelper(dbHelper);
        MapData.getInstance().setDbHelper(dbHelper);
        ExhibitsData.getInstance().setDbHelper(dbHelper);
        ReadyRaports.getInstance().setDbHelper(dbHelper);
        try {
            if (!dataLoaded) {
                MapData.getInstance().loadDbData();
                ExhibitsData.getInstance().loadDbData();
                ReadyRaports.getInstance().loadDbData();
                NetworkHandler.getInstance().uploadRaports();
                dataLoaded = true;
            }
        } catch (DatabaseLoadException e) {
            e.printStackTrace();
            downloadMap();
        }
    }

    @Override
    public void onBackPressed() {

    }

    public void mapClick(View view) {
        downloadMap();
    }

    public void resetClick(View view) {
        dbHelper.addOrUpdateExhibits(null, new ArrayList<>());
        Log.i(LOG_TAG, "Exhibits reset");
    }

    public void surveyClick(View view) {
        Intent i = new Intent(getApplicationContext(), SurveyActivity.class);
        i.putExtra("nextActivity", MapActivity.class);
        i.putExtra("type", Survey.SurveyType.BEFORE);
        startActivity(i);
    }

    private void downloadMap() {
        setProgressBar();
        MapData.getInstance().downloadMap(this::synchronizationFailed, this::mapDownloaded);
    }

    private void mapDownloaded() {
        if (!dataLoaded) {
            try {
                MapData.getInstance().loadDbData();
                ExhibitsData.getInstance().loadDbData();
                ReadyRaports.getInstance().loadDbData();
                NetworkHandler.getInstance().uploadRaports();
                dataLoaded = true;
            } catch (DatabaseLoadException e) {
                progressBar.dismiss();
                e.printStackTrace();
                runOnUiThread(this::showAlert);
            }
        }
        progressBar.dismiss();
    }

    private void showAlert() {
        DialogInterface.OnClickListener positiveAction = (dialog, which) -> downloadMap();
        DialogInterface.OnClickListener negativeAction = (dialog, which) -> System.exit(1);
        Utils.showDialog(this,
                         R.string.dataError,
                         R.string.tryAgain,
                         R.string.exit,
                         positiveAction,
                         negativeAction);
    }

    private void synchronizationFailed() {
        runOnUiThread(() -> {
            progressDialogUpdate.interrupt();
            progressBar.dismiss();

            DialogInterface.OnClickListener positiveAction = (dialog, which) -> downloadMap();
            Utils.showDialog(this,
                             R.string.synchronizationFailed,
                             R.string.tryAgain,
                             R.string.cancel,
                             positiveAction);
        });
    }

    private void setProgressBar() {
        progressBar = new ProgressDialog(this);
        progressBar.setCanceledOnTouchOutside(false);
        progressBar.setCancelable(false);
        progressBar.setTitle(getString(R.string.synchronizing));
        progressBar.setMessage(getString(R.string.synchronizingInProgress));
        progressBar.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        progressBar.setProgressNumberFormat(null);
        progressBar.setProgress(0);
        progressBar.setMax(100);
        progressBar.show();

        final Integer[] progressBarProgress = {0};

        progressDialogUpdate = new Thread(() -> {
            while (progressBarProgress[0] < 100) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                    return;
                }

                if (MapData.getInstance().getMapDownloadedTilesCount() != null &&
                    MapData.getInstance().getMapDownloadingTilesCount() != null) {
                    progressBarProgress[0] =
                            100 * MapData.getInstance().getMapDownloadedTilesCount() /
                            MapData.getInstance().getMapDownloadingTilesCount();
                }

                runOnUiThread(() -> progressBar.setProgress(progressBarProgress[0]));
            }
        });
        progressDialogUpdate.start();
    }
}

