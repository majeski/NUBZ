package com.cnk.communication.task;

import android.util.Log;

import com.cnk.communication.thrift.RawReport;
import com.cnk.communication.thrift.RawReportEvent;
import com.cnk.communication.thrift.Server;
import com.cnk.data.DataHandler;
import com.cnk.data.raport.Raport;
import com.cnk.data.raport.RaportEvent;
import com.cnk.notificators.Notificator;

import org.apache.thrift.TException;

import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class RaportUploadTask extends ServerTask {

    private static final String LOG_TAG = "RaportUploadTask";

    public RaportUploadTask(Notificator notificator) {
        super(notificator);
    }

    @Override
    protected void performInSession(Server.Client client) throws TException {
        Map<Raport, Integer> toSend = DataHandler.getInstance().getAllReadyRaports();
        for (Map.Entry<Raport, Integer> entry : toSend.entrySet()) {
            Raport raport = entry.getKey();
            Integer serverId = entry.getValue();
            if (serverId == null) {
                serverId = client.getIdForNewReport();
                DataHandler.getInstance().setServerId(raport, serverId);
            }
            Log.i(LOG_TAG, "Sending raport with id " + raport.getId().toString());
            RawReport thriftRaport = translateToThrift(raport, serverId);
            client.saveReport(thriftRaport);
            Log.i(LOG_TAG, "Raport sent");
            DataHandler.getInstance().markRaportAsSent(raport);
        }
    }

    private RawReport translateToThrift(Raport raport, Integer serverId) {
        List<RawReportEvent> events = new LinkedList<>();
        for (RaportEvent event : raport.getHistory()) {
            RawReportEvent rawEvent = new RawReportEvent();
            rawEvent.setActions(event.getActions());
            rawEvent.setDurationInSecs(event.getDurationSeconds());
            if (event.getExhibitId() != null) {
                rawEvent.setExhibitId(event.getExhibitId());
            }
            events.add(rawEvent);
        }
        RawReport rawRaport = new RawReport();
        rawRaport.setHistory(events);
        rawRaport.setReportId(serverId);
        return rawRaport;
    }

}
