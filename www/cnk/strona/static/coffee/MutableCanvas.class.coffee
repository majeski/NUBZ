root = exports ? this
root.MutableCanvas = class MutableCanvas extends root.Canvas
  constructor: ->
    super
    @changeMapButton = L.easyButton('fa-map-o',
                                    => @fireEvents "mapChangeRequest",
                                    'Zmień obrazek piętra',
                                    {position: 'bottomleft'}).addTo(@_map)

  addExhibits: (floor, exhibits) =>
    for idx, e of exhibits
      continue unless e.frame?.mapLevel is floor
      X = e.frame.x
      Y = e.frame.y
      polygonBounds = [
        @_map.unproject([X, Y], 3),
        @_map.unproject([X, Y + e.frame.width], 3),
        @_map.unproject([X + e.frame.height, Y + e.frame.height], 3),
        @_map.unproject([X + e.frame.height, Y], 3)
      ]
      r = L.polygon(polygonBounds, {
          color: "#ff7800"
          weight: 1
          draggable: true
          id: idx
        }).bindLabel(e.name, {
          direction: 'auto'
        })
      r.on('dragstart', @_onDragStart)
      #IDEA (in case nothing to do): move bouncing from boundaries to @_onDrag
      # plus somehow refresh event's latlangs in @_onDrag (leaflet or L.Path.Drag problem?)
      r.on('dragend', @_onDragEnd)
      @_exhibits[floor].addLayer(r)
    @

  _onDragStart: (e) ->
    e.target.hideLabel()
    e.target.bringToFront()
    @

  _onDragEnd: (e) =>
    @_fixExhibitPosition(e)
    exhibit = e.target
    exhibitId = exhibit.options.id
    geoPoint = exhibit.getLatLngs()[0][0]
    scaledPoint = @_map.project(geoPoint, @mapData.maxZoom[@mapData.activeFloor])
    @_changeExhibitPositionRequest(exhibitId, scaledPoint)
    if @_labelsButton._currentState.stateName is 'removeLabels'
      exhibit.showLabel()
    @

  _fixExhibitPosition: (e) =>
    exhibit = e.target
    maxX = @_map.project(@_mapBounds[@mapData.activeFloor].getNorthEast()).x
    maxY = @_map.project(@_mapBounds[@mapData.activeFloor].getSouthWest()).y

    #points are sorted in clockwise order, starting with top left point
    latLng = exhibit.getLatLngs()[0]
    exhibitPoints = (@_map.project(ll) for ll in latLng)

    #update points in edge cases
    [dx, dy] = @_getExhibitProtrusion(exhibitPoints[0], exhibitPoints[2], maxX, maxY)
    exhibitPoints = (new L.Point(p.x + dx, p.y + dy) for p in exhibitPoints)
    newGeoPoints = (@_map.unproject(p) for p in exhibitPoints)
    exhibit.setLatLngs(newGeoPoints)

  _getExhibitProtrusion: (topLeft, bottomRight, maxX, maxY) ->
    if topLeft.x < 0
      dx = -topLeft.x
    else if bottomRight.x > maxX
      dx = maxX - bottomRight.x
    if topLeft.y < 0
      dy = -topLeft.y
    else if bottomRight.y > maxY
      dy = maxY - bottomRight.y
    [dx ? 0, dy ? 0]

  _changeExhibitPositionRequest: (id, topLeft) =>
    toSend = {
      jsonData:
        JSON.stringify(
          id: id
          x: topLeft.x
          y: topLeft.y
        )
    }
    instance = @
    handler = @_ajaxSuccessHandler
    jQuery.ajaxSetup(
      headers: { "X-CSRFToken": getCookie("csrftoken") }
    )
    jQuery.ajax(
      type: 'POST'
      dataType: 'json'
      url: '/updateExhibitPosition/'
      data: toSend
      success: handler.bind(instance)
    )

  _ajaxSuccessHandler: (data) =>
    if data.success is true
      @mapData.exhibits[data.id].frame.x = data.x
      @mapData.exhibits[data.id].frame.y = data.y
    else
      @refresh()
      BootstrapDialog.alert(
        message: '<p align="center">Wystąpił nieoczekiwany błąd. Spróbuj ponownie.</p>'
        type: BootstrapDialog.TYPE_DANGER
        title: 'Błąd serwera'
      )