package com.cnk.communication.task;

import android.util.Log;

import com.cnk.communication.Server;
import com.cnk.notificators.Notificator;
import com.cnk.utilities.Util;

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransport;

public abstract class ServerTask extends Task {

    protected static final String LOG_TAG = "ServerTask";
    protected long delay = 1;
    private static final String SEND_ADDRESS = "192.168.0.5";
    private static final int SEND_PORT = 9090;

    protected Notificator notificator;

    public ServerTask(Notificator notificator) {
        this.notificator = notificator;
    }

    @Override
    public void run(int tries) {
        Log.i(LOG_TAG, "Starting run");
        TTransport socket = openSocket(tries);
        TProtocol protocol = new TBinaryProtocol(socket);
        Server.Client client = new Server.Client(protocol);
        while (tries > 0 && socket != null) {
            try {
                performInSession(client);
                notificator.success();
                Log.i(LOG_TAG, "Action successful");
                socket.close();
                return;
            } catch (TException e) {
                Log.e(LOG_TAG, "Action failed, remaining tries: " + Integer.toString(tries));
                e.printStackTrace();
                Util.waitDelay(delay * 1000);
                delay += 2;
                tries--;
            }
        }
        notificator.failure();
        if (socket != null) {
            socket.close();
        }

    }

    protected abstract void performInSession(Server.Client client) throws TException;

    private TTransport openSocket(Integer tries) {
        TTransport socket = new TSocket(SEND_ADDRESS, SEND_PORT);
        Log.i(LOG_TAG, "Opening socket");
        while (tries > 0) {
            try {
                socket.open();
                Log.i(LOG_TAG, "Opened socket");
                return socket;
            } catch (org.apache.thrift.transport.TTransportException transportException) {
                tries--;
                Log.e(LOG_TAG, "Socket open failed, remaining tries: " + Integer.toString(tries));
                Util.waitDelay(delay * 1000);
                delay += 2;
            }
        }
        return null;
    }

}