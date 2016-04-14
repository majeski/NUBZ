package com.cnk.activities;

import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;

import com.cnk.R;
import com.cnk.communication.task.ServerTask;
import com.cnk.communication.task.Task;
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
        MapData.getInstance().downloadMap(this::mapDownloaded, this::synchronizationFailed);
    }

    private void mapDownloaded(Task t) {
        Log.i(LOG_TAG, "Task: " + t.getTaskName() + " finished sucessfully.");
        if (!dataLoaded) {
            try {
                MapData.getInstance().loadDbData();
                ExhibitsData.getInstance().loadDbData();
                ReadyRaports.getInstance().loadDbData();
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

    private void synchronizationFailed(Task t, ServerTask.FailureReason failureReason) {
        Log.i(LOG_TAG, "Task: " + t.getTaskName() + " failed. Reason: " + failureReason.toString());
        runOnUiThread(() -> {
            progressBar.dismiss();

            DialogInterface.OnClickListener positiveAction = (dialog, which) -> downloadMap();
            DialogInterface.OnClickListener
                    negativeAction =
                    dataLoaded ? null : (dialog, which) -> finish();
            Utils.showDialog(this,
                             com.cnk.utilities.Util.downloadErrorMessage(failureReason),
                             getString(R.string.tryAgain),
                             getString(R.string.cancel),
                             positiveAction,
                             negativeAction);
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

        MapData.getInstance().addObserver(this, () -> {
            if (progressBar.isShowing()) {
                progressBar.setProgress(progressBar.getProgress() + 1);
            }
        });
    }
}

