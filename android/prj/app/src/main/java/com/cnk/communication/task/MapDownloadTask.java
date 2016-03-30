package com.cnk.communication.task;

import android.util.Log;

import com.cnk.communication.NetworkHandler;
import com.cnk.communication.thrift.MapImage;
import com.cnk.communication.thrift.NewMapImagesRequest;
import com.cnk.communication.thrift.NewMapImagesResponse;
import com.cnk.communication.thrift.Server;
import com.cnk.communication.thrift.Size;
import com.cnk.data.map.FloorMap;
import com.cnk.data.map.MapData;
import com.cnk.data.map.Resolution;
import com.cnk.data.map.ZoomLevel;

import org.apache.thrift.TException;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;


public class MapDownloadTask extends ServerTask {

    private static final String LOG_TAG = "MapDownloadTask";
    private MapData.MapUpdateAction action;


    public MapDownloadTask(NetworkHandler.FinishAction failure,
                           NetworkHandler.FinishAction success,
                           MapData.MapUpdateAction action) {
        super(failure, success);
        this.action = action;
    }

    public void performInSession(Server.Client client) throws TException, IOException {
        downloadTilesUpdate(client);
        Log.i(LOG_TAG, "Map update complete");
        action.doOnUpdate();
        action = null;
    }

    private void downloadTilesUpdate(Server.Client client) throws TException, IOException {
        Log.i(LOG_TAG, "Downloading map tiles addresses");

        NewMapImagesRequest request = new NewMapImagesRequest();
        NewMapImagesResponse response = client.getNewMapImages(request);
        Log.i(LOG_TAG, "Map tiles addresses downloaded");

        List<FloorMap> maps = new ArrayList<>();
        for (Map.Entry<Integer, MapImage> e : response.getFloors().entrySet()) {
            maps.add(translateFromThrift(e.getKey(), e.getValue()));
        }
        MapData.getInstance().setMaps(maps);
    }

    private FloorMap translateFromThrift(Integer floor, MapImage thriftResponse) {
        Size thriftSize = thriftResponse.getResolution();
        Resolution originalSize = new Resolution(thriftSize.getWidth(), thriftSize.getHeight());
        List<com.cnk.communication.thrift.ZoomLevel>
                imageTilesThrift =
                thriftResponse.getZoomLevels();

        ArrayList<ZoomLevel> zoomLevels = new ArrayList<>();
        for (com.cnk.communication.thrift.ZoomLevel tile : imageTilesThrift) {
            Resolution
                    scaledSize =
                    new Resolution(tile.getScaledSize().getWidth(),
                                   tile.getScaledSize().getHeight());
            Resolution
                    tileSize =
                    new Resolution(tile.getTileSize().getWidth(), tile.getTileSize().getHeight());
            List<List<String>> toCopy = tile.getTilesUrls();
            ZoomLevel toAdd = new ZoomLevel(scaledSize, tileSize, copyThriftList(toCopy));
            zoomLevels.add(toAdd);
        }

        return new FloorMap(floor, originalSize, zoomLevels);
    }

    private ArrayList<ArrayList<String>> copyThriftList(List<List<String>> thriftList) {
        ArrayList<ArrayList<String>> copied = new ArrayList<>();
        for (List<String> list : thriftList) {
            ArrayList<String> toAdd = new ArrayList<>();
            for (String s : list) {
                toAdd.add(s);
            }
            copied.add(toAdd);
        }
        return copied;
    }
}
