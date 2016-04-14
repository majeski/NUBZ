package com.cnk.data.exhibits;

import com.cnk.database.DatabaseHelper;
import com.cnk.database.models.Exhibit;
import com.cnk.database.models.Version;
import com.cnk.exceptions.DatabaseLoadException;
import com.cnk.notificators.Observable;
import com.cnk.utilities.Consts;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ExhibitsData extends Observable<ExhibitsData.ExhibitsUpdateAction> {
    public interface ExhibitsUpdateAction {
        void doOnUpdate(List<Exhibit> changedExhibits);
    }

    private static ExhibitsData instance;
    private Integer exhibitsVersion;
    private DatabaseHelper dbHelper;
    private List<FloorExhibitsInfo> floorInfos;

    private ExhibitsData() {
        floorInfos = new ArrayList<>();
        exhibitsVersion = null;
    }

    public static ExhibitsData getInstance() {
        if (instance == null) {
            instance = new ExhibitsData();
        }
        return instance;
    }

    public void notifyObservers(List<Exhibit> changedExhibits) {
        List<ExhibitsUpdateAction> actions = new ArrayList<>();
        for (ExhibitsUpdateAction action : observers.values()) {
            actions.add(action);
        }

        for (ExhibitsUpdateAction action : actions) {
            action.doOnUpdate(changedExhibits);
        }
    }

    public void setDbHelper(DatabaseHelper dbHelper) {
        this.dbHelper = dbHelper;
    }

    public void loadDbData() throws DatabaseLoadException {
        exhibitsVersion = dbHelper.getVersion(Version.EXHIBITS);
        try {
            floorInfos.clear();
            for (int floor = 0; floor < Consts.FLOOR_COUNT; floor++) {
                FloorExhibitsInfo currentFloor = new FloorExhibitsInfo();
                currentFloor.setExhibits(exhibitListToMap(dbHelper.getAllExhibitsForFloor(floor)));
                floorInfos.add(currentFloor);
            }
        } catch (Exception e) {
            e.printStackTrace();
            throw new DatabaseLoadException();
        }
    }

    private Map<Integer, Exhibit> exhibitListToMap(List<Exhibit> exhibitList) {
        Map<Integer, Exhibit> map = new HashMap<>();
        for (Exhibit e : exhibitList) {
            map.put(e.getId(), e);
        }
        return map;
    }

    public void setExhibits(List<Exhibit> exhibits, Integer version) {
        if (exhibits.isEmpty()) {
            return;
        }
        dbHelper.addOrUpdateExhibits(version, exhibits);
        exhibitsVersion = version;
        updateExhibits(exhibits);
        notifyObservers(exhibits);
    }

    private void updateExhibits(List<Exhibit> newExhibits) {
        for (Exhibit newExhibit : newExhibits) {
            findAndChangeExhibit(newExhibit);
        }
    }

    private void findAndChangeExhibit(Exhibit e) {
        for (FloorExhibitsInfo currentFloor : floorInfos) {
            currentFloor.removeExhibit(e.getId());
        }
        if (e.getFloor() != null) {
            floorInfos.get(e.getFloor()).addExhibit(e);
        }
    }

    public List<Exhibit> getExhibitsOfFloor(int floor) {
        return floorInfos.get(floor).getExhibitsList();
    }

    public Integer getExhibitsVersion() {
        return exhibitsVersion;
    }

}
