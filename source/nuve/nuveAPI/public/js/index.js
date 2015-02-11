'use strict';

var nuve;
var mode = "";
var metadata;
var ENUMERATE = {
  SERVICE: "service",
  ROOM: "room",
  RUNTIME: "runtime"
};
var serviceId = "";
var serviceKey = "";

function checkProfile(callback) {
  var serviceId = getCookie('serviceId') === '' ? top.serviceId : getCookie('serviceId');
  var serviceKey = getCookie('serviceKey') === '' ? top.serviceKey : getCookie('serviceKey');
  top.serviceId = serviceId;
  $('.modal-body #inputId').val(serviceId);
  $('.modal-body #inputKey').val(serviceKey);
  if (serviceId === '' || serviceKey === '') {
    $('button#profile').click();
    return;
  }
  nuve = Nuve.init(serviceId, serviceKey);
  judgePermissions();
  callback();
}

$('button#clearCookie').click(function() {
  document.cookie = 'serviceId=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  document.cookie = 'serviceKey=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  document.getElementById("inputId").value = "";
  document.getElementById("inputKey").value = "";
});

$('button#saveSericeInfo').click(function() {
  serviceId = $('.modal-body #inputId').val();
  serviceKey = $('.modal-body #inputKey').val();
  var rememberMe = $('.modal-body .checkbox input').prop('checked');
  if (serviceId !== '' && serviceKey !== '') {
    if (rememberMe) {
      setCookie('serviceId', serviceId, 365);
      setCookie('serviceKey', serviceKey, 365);
    }
    nuve = Nuve.init(serviceId, serviceKey);
    judgePermissions();
  }
  if (nuve) {
    $("#myModal").modal("hide");
  }
});

function judgePermissions() {
  nuve.getServices(function(err, text) {
    if (err !== 401) {
      $(".li:not(.active)").removeClass("hideLi");
    } else {
      $(".li:not(.active)").addClass("hideLi");
      $(".overview").hide();
      $(".room").show();
      $(".runtime").hide();
      $(".page-header").text("Rooms in current Service");
      mode = ENUMERATE.ROOM;
    }
  });
}

function ServiceCreation() {
  this.toggle = function() {
    $('#myModal2').modal('toggle');
  };
  $('#myModal2 .modal-footer button:first').click(function() {
    var serviceName = $('#cs-form #cs-name').val();
    var serviceKey = $('#cs-form #cs-key').val();
    if (serviceName !== '' && serviceKey !== '') {
      nuve.createService(serviceName, serviceKey, function(err, text) {
        if (err) return notify('error', 'Service Creation', err);
        notify('info', 'Service Creation', text);
        renderServices();
      });
    } else {
      notify('error', 'Service Creation', 'invalid service name or key');
    }
  });
  $('#myModal2').on('hidden.bs.modal', function() {
    $('#cs-form #cs-name').val('');
    $('#cs-form #cs-key').val('');
  });
}

var serviceCreation = new ServiceCreation();

$('img#createService').click(function() {
  // toggle service creation form
  serviceCreation.toggle();
});

var tableTemplateService = '{{#rooms}}<tr>\
      <td><code>{{_id}}</code></td>\
      <td>{{name}}</td>\
      <td>{{mode}}</td>\
      <td>{{publishLimit}}</td>\
      <td>{{userLimit}}</td>\
      <td>{{mediaSetting}}</td>\
    </tr>{{/rooms}}';
Mustache.parse(tableTemplateService);

function tableHandlerService(rooms, sname) {
  var template = tableTemplateService;
  var view = Mustache.render(template, {
    rooms: rooms,
    mediaSetting: function() {
      if (this.mediaMixing === null) return 'default';
      return 'customized';
    }
  });
  $('.overview #serviceTable tbody').html(view);
  $('h3#tableTitle').html('Rooms in service <em class="text-primary">' + sname + '</em>');
}

var graphicTemplateService = '{{#services}}<div class="col-xs-6 col-sm-3 placeholder" id="service_{{_id}}">\
    <a href="#"><img class="img-responsive" src="img/service.svg" width="50%"></a>\
    <h4>{{name}}</h4>\
    <code>{{_id}}</code><button type="button" class="close" aria-label="Close"><span aria-hidden="true">&times;</span></button><br />\
    <span>{{rooms.length}} rooms</span>\
  </div>{{/services}}';
Mustache.parse(graphicTemplateService);

function graphicHandlerService(services) {
  var view = Mustache.render(graphicTemplateService, {
    services: services
  });
  $('div#serviceGraph').html(view);
  services.map(function(service) {
    var id = service._id;
    $('div#service_' + id + ' img').click(function() {
      tableHandlerService(service.rooms, service.name);
    });
    if(id === serviceId) $('div#service_' + id + ' button.close').hide();
    $('div#service_' + id + ' button.close').click(function() {
      notifyConfirm('Delete Service', 'Are you sure want to delete service ' + id, function() {
        nuve.deleteService(id, function(err, resp) {
          if (err) return notify('error', 'Delete Service Error', resp);
          renderServices();
          notify('info', 'Delete Service Success', resp);
        });
      });
    });
  });
}

function renderServices() {
  nuve.getServices(function(err, text) {
    if (err) {
      return notify('error', 'Get Services', err);
    }
    try {
      var obj = JSON.parse(text);
      graphicHandlerService(obj);
    } catch (e) {
      notify('error', 'Service Creation', e);
    }
  });
}

var tableTemplateRoom = '{{#rooms}}<tr>\
      <td><code>{{_id}}</code></td>\
      <td class="roomName" data-spin="name">{{name}}</td>\
      <td class="roomMode" data-spin="mode" data-value="{{mode}}"></td>\
      <td class="publishLimit" data-spin="publishLimit">{{publishLimit}}</td>\
      <td class="userLimit" data-spin="userLimit">{{userLimit}}</td>\
      <td class="mediaSetting" data-spin="mediaMixing">object</td>\
      <td class="col-md-1"><button type="button" id="apply-room" class="btn btn-xs btn-primary">Apply</button> <button type="button" id="delete-room" class="btn btn-xs btn-danger">Delete</button></td>\
    </tr>{{/rooms}}';
Mustache.parse(tableTemplateRoom);

function tableHandlerRoom(rooms) {
  var roomCache = {};
  rooms.map(function(room) {
    roomCache[room._id] = room;
  });

  var template = tableTemplateRoom;
  var view = Mustache.render(template, {
    rooms: rooms
  });
  $('.room #serviceTable tbody').html(view);
  $('.room #serviceTable tbody').append('<tr>\
      <td></td>\
      <td class="roomName" data-spin="name"></td>\
      <td class="roomMode" data-spin="mode"></td>\
      <td class="publishLimit" data-spin="publishLimit"></td>\
      <td class="userLimit" data-spin="userLimit"></td>\
      <td class="mediaSetting" data-spin="mediaMixing"></td>\
      <td class="col-md-1"><button type="button" id="add-room" class="btn btn-xs btn-success">Add</button> <button type="button" id="reset-room" class="btn btn-xs btn-warning">Reset</button></td>\
    </tr>');

  var applyRoomFn = function() {
    var p = $(this).parent().parent();
    var unsaved = p.find('.editable-unsaved');
    if (unsaved.length === 0) {
      return notify('warning', 'Update Room', 'nothing to update');
    }
    var roomId = p.find('td:first').text();
    if (roomId !== '') {
      var updates = {};
      unsaved.map(function(index, each) {
        var key = $(each).attr('data-spin');
        var valueObj = $(each).editable('getValue');
        var id = $(each).attr('id');
        updates[key] = valueObj[id];
      });
      nuve.updateRoom(roomId, updates, function(err, resp) {
        if (err) return notify('error', 'Update Room', resp);
        setTimeout(function() {
          p.find('.editable-unsaved').removeClass('editable-unsaved');
        }, 20);
        roomCache[roomId] = JSON.parse(resp);
        notify('info', 'Update Room Success', roomId);
      });
    } else {
      notify('error', 'Update Room', 'error in finding roomId');
    }
  };
  var deleteRoomFn = function() {
    var p = $(this).parent().parent();
    var roomId = p.find('td:first').text();
    if (roomId !== '') {
      notifyConfirm('Delete Room', 'Are you sure want to delete room ' + roomId, function() {
        nuve.deleteRoom(roomId, function(err, resp) {
          if (err) return notify('error', 'Delete Room', resp);
          p.remove();
          delete roomCache[roomId];
          notify('info', 'Delete Room Success', resp);
        });
      });
    } else {
      notify('error', 'Delete Room', 'error in finding roomId');
    }
  };

  $('button#apply-room').click(applyRoomFn);
  $('button#delete-room').click(deleteRoomFn);

  var roomNameHandle = {
    mode: 'inline',
    validate: function(value) {
      var val = $.trim(value);
      if (val === '') return 'This field is required';
      return {
        newValue: val
      };
    }
  };
  var roomModeHandle = {
    mode: 'inline',
    type: 'select',
    source: metadata.mode.map(function(mode) {
      return {
        value: mode,
        text: mode
      };
    })
  };
  var numberHandle = {
    mode: 'inline',
    validate: function(value) {
      var val = parseInt(value, 10);
      if (isNaN(val) || val < 0) return 'value should be a number in [0, +Infinity)';
      return {
        newValue: val
      };
    }
  };
  var numberHandle1 = {
    mode: 'inline',
    validate: function(value) {
      var val = parseInt(value, 10);
      if (isNaN(val) || val < -1) return 'value should be a number in [-1, +Infinity)';
      return {
        newValue: val
      };
    }
  };
  var disabledHandle = {
    disabled: true
  };

  var mediaSettingFn = function() {
    if (this.parentNode.children[0].innerText === "") return;
    $('#myModal3').modal('toggle');
    var roomId = $(this).parent().find('td:first').text();
    var room = roomCache[roomId];
    if (typeof room === 'undefined') {
      return notify('error', 'Media Setting', 'error in finding roomId');
    }
    var p = $(this);
    $('#myModal3 .modal-title').text('Media Setting for Room ' + roomId);
    var videoSetting = (room.mediaMixing || {}).video || {
      resolution: 'vga',
      bitrate: 0,
      bkColor: 'black',
      avCoordinated: false,
      maxInput: 16,
      layout: {
        base: 'fluid',
        custom: []
      }
    };

    var view = Mustache.render('<tr>\
          <td rowspan="7">video</td>\
          <td colspan="2">resolution</td>\
          <td id="resolution" class="value-num-edit" data-value={{resolution}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">bitrate</td>\
          <td id="bitrate" class="value-num-edit" data-value={{bitrate}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">bkColor</td>\
          <td id="bkColor" class="value-num-edit" data-value={{bkColor}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">maxInput</td>\
          <td id="maxInput" class="value-num-edit" data-value={{maxInput}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">avCoordinated</td>\
          <td id="avCoordinated" class="value-num-edit" data-value={{avCoordinated}}></td>\
        </tr>\
        <tr>\
          <td rowspan="2">layout</td>\
          <td>base</td>\
          <td id="layout.base" class="value-num-edit" data-value={{layout.base}}></td>\
        </tr>\
        <tr>\
          <td>custom</td>\
          <td id="layout.custom" class="value-obj-edit" data-type="textarea"></td>\
        </tr>\
        <tr>\
          <td>audio</td>\
          <td colspan="3" class="text-muted"><em>NOT SUPPORTED NOW</em></td>\
        </tr>', videoSetting);

    $('#myModal3 #inRoomTable tbody').html(view);
    $('#myModal3 .modal-footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal"><i class="glyphicon glyphicon-ok"></i></button>\
        <button type="button" class="btn btn-default" data-dismiss="modal"><i class="glyphicon glyphicon-remove"></i></button>');
    $('#myModal3 tbody td#resolution').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.resolution.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });
    $('#myModal3 tbody td#bitrate').editable(numberHandle);
    $('#myModal3 tbody td#maxInput').editable(numberHandle);
    $('#myModal3 tbody td#bkColor').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.bkColor.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });
    $('#myModal3 tbody td#avCoordinated').editable({
      mode: 'inline',
      type: 'select',
      source: [{
        value: 0,
        text: 'false'
      }, {
        value: 1,
        text: 'true'
      }]
    });
    $('#myModal3 tbody td.value-num-edit:last').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.layout.base.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });
    $('#myModal3 tbody td.value-obj-edit').editable({
      title: 'Input a stringified JSON object',
      validate: function(value) {
        value = $.trim(value);
        if (value !== '') {
          try {
            JSON.parse(value);
          } catch (e) {
            return 'invalid input';
          }
        }
        return {
          newValue: value
        };
      },
      value: JSON.stringify(videoSetting.layout.custom, null, 2),
    });
    // <audio not supported now>
    $('#myModal3 .modal-footer button:first').click(function() {
      var unsaved = $('#myModal3 tbody .editable-unsaved');
      if (unsaved.length === 0) return;
      unsaved.map(function(index, each) {
        var id = $(each).attr('id');
        var val = $(each).editable('getValue')[id];
        setVal(videoSetting, id, val);
      });
      room.mediaMixing = room.mediaMixing || {};
      room.mediaMixing.video = videoSetting;
      p.addClass('editable-unsaved');
      p.editable('setValue', room.mediaMixing);
      p.text('object');
    });
  };

  $('td.roomName').editable(roomNameHandle);
  $('td.roomMode').editable(roomModeHandle);
  $('td.publishLimit').editable(numberHandle1);
  $('td.userLimit').editable(numberHandle1);
  $('td.mediaSetting').editable(disabledHandle);
  $('td.mediaSetting').click(mediaSettingFn);

  $('#myModal3').on('hidden.bs.modal', function() {
    $('#myModal3 #inRoomTable tbody').empty();
  });

  $('button#add-room').click(function() {
    var p = $(this).parent().parent();
    if (p.find('td.roomName').hasClass('editable-empty')) {
      return notify('error', 'Add Room', 'Empty room name');
    }
    var roomName = p.find('td.roomName').text();
    var roomMode = p.find('td.roomMode').editable('getValue').roomMode;
    var publishLimit = parseInt(p.find('td.publishLimit').text(), 10);
    var userLimit = parseInt(p.find('td.userLimit').text(), 10);
    var room = {
      name: roomName,
      options: {
        mode: roomMode,
        publishLimit: publishLimit,
        userLimit: userLimit,
        mediaMixing: null
      }
    };
    p.find('.editable-unsaved').editable('setValue', null).removeClass('editable-unsaved'); // reset line
    nuve.createRoom(room, function(err, resp) {
      if (err) return notify('error', 'Add Room', resp);
      var room1 = JSON.parse(resp);
      var view = Mustache.render(tableTemplateRoom, {
        rooms: [room1]
      });
      $(view).insertBefore(p.parent().find('tr:last'));
      var selector = p.parent().find('tr:nth-last-child(2)');
      selector.find('td.roomName').editable(roomNameHandle);
      selector.find('td.roomMode').editable(roomModeHandle);
      selector.find('td.publishLimit').editable(numberHandle1);
      selector.find('td.userLimit').editable(numberHandle1);
      selector.find('td.mediaSetting').editable(disabledHandle);
      selector.find('td.mediaSetting').click(mediaSettingFn);
      selector.find('button#apply-room').click(applyRoomFn);
      selector.find('button#delete-room').click(deleteRoomFn);
      roomCache[room1._id] = room1;
      notify('info', 'Add Room Success', room1._id);
    });
  });

  $('button#reset-room').click(function() {
    var p = $(this).parent().parent();
    p.find('.editable-unsaved').editable('setValue', null).removeClass('editable-unsaved');
  });
}

function renderRooms() {
  nuve.getRooms(function(err, resp) {
    if (err) return notify('error', 'Room Management', err);
    var rooms = JSON.parse(resp);
    tableHandlerRoom(rooms);
  });
}

function setVal(obj, key, val) {
  key = key.split('.');
  while (key.length > 1)
    obj = obj[key.shift()];
  obj[key.shift()] = val;
}

$('#myModal').on('hidden.bs.modal', function() { // handle profile setting applying
  switch (mode) {
    case ENUMERATE.SERVICE:
      renderServices();
      break;
    case ENUMERATE.ROOM:
      renderRooms();
      break;
    case ENUMERATE.RUNTIME:
      renderCluster();
      break;
    default:
      renderRooms();
      break;
  }
});

var tableTemplateRunTime = '{{#nodes}}' + $('div#mcu-ecs-table tbody').html() + '{{/nodes}}';
Mustache.parse(tableTemplateRunTime);

function tableHandlerRunTime(nodes) {
  var template = tableTemplateRunTime;
  var view = Mustache.render(template, {
    nodes: nodes,
    address: function() {
      if (this.hostname !== '') return this.hostname;
      return this.ip;
    }
  });
  $('div#mcu-ecs-table tbody').html(view);
}

var graphicTemplateRunTime = '{{#nodes}}<div class="col-xs-6 col-sm-3 placeholder" id="{{rpcID}}">\
    <a href="#"><img class="img-responsive" src="img/node.svg" width="33%"></a>\
    <h4 class="text-{{status}}">{{rpcID}}</h4>\
  </div>{{/nodes}}';
Mustache.parse(graphicTemplateRunTime);
var configTableTemplate = '<tr>\
      <th>{{defaultVideoBW}}</th>\
      <th>{{maxVideoBW}}</th>\
      <th>{{turn}}</th>\
      <th>{{stun}}</th>\
      <th>{{warning_n_rooms}}</th>\
      <th>{{limit_n_rooms}}</th>\
    </tr>';
Mustache.parse(configTableTemplate);

function graphicHandlerRunTime(nodes) {
  var view = Mustache.render(graphicTemplateRunTime, {
    nodes: nodes,
    status: function() {
      if (this.state === 0) return 'danger';
      else if (this.state === 1) return 'warning';
      return 'success';
    }
  });
  $('div#mcu-ecs').html(view);
  var nodeConfHandler = (function() {
    var opened = false;
    return function() {
      if (opened) {
        opened = false;
        return $('div#ec-config-table').hide();
      }
      opened = true;
      var nodeId = $(this).attr('id');
      nuve.getClusterNodeConfig(nodeId, function(err, resp) {
        if (err) return notify('error', 'Get Node Configuration', err);
        var config = JSON.parse(resp);
        config.turn = function() {
          if (typeof this.turnServer === 'object') return JSON.stringify(this.turnServer);
          return '' + this.turnServer;
        };
        config.stun = function() {
          if (typeof this.stunServerUrl === 'object') return JSON.stringify(this.stunServerUrl);
          return '' + this.stunServerUrl;
        };
        $('div#ec-config-table tbody').html(Mustache.render(configTableTemplate, config));
        $('div#ec-config-table #tableTitle').html('Configuration for <em class="text-primary">' + nodeId + '</em>');
        $('div#ec-config-table').show();
      });
    };
  }());
  $('div#mcu-ecs').children().click(nodeConfHandler);
}

function renderCluster() {
  nuve.getClusterNodes(function(err, resp) {
    if (err) return notify('error', 'Cluster Info', err);
    var nodes = JSON.parse(resp);
    tableHandlerRunTime(nodes.map(function(node, index) {
      node.index = index;
      return node;
    }));
    graphicHandlerRunTime(nodes);
  });
}

window.onload = function() {
  $(".modal-backdrop").css("height", document.body.scrollHeight);
  $(".close").on("click", function() {
    if (serviceId === '' || serviceKey === '') {
      return;
    } else {
      $("#myModal").modal("hide");
    }
  });
  $.getJSON('meta.json').done(function(data) {
    metadata = data;
  }).fail(function() {
    notify('error', 'Loading', 'Error in fetching data');
  }).complete(function() {
    checkProfile(renderRooms);
  });
};

function a_click(nowList, dom) {
  var service = $(".overview");
  var room = $(".room");
  var runtime = $(".runtime");
  var nowLI = $(dom.parentNode);
  var title = $(".page-header");
  if (nowLI.hasClass("active")) {
    return;
  } else {
    $(".li").removeClass("active");
    nowLI.addClass("active");
  }
  switch (nowList) {
    case ENUMERATE.SERVICE:
      service.show();
      room.hide();
      runtime.hide();
      title.text("Services");
      checkProfile(renderServices);
      break;
    case ENUMERATE.ROOM:
      service.hide();
      room.show();
      runtime.hide();
      title.text("Rooms in current Service");
      $.getJSON('meta.json').done(function(data) {
        metadata = data;
      }).fail(function() {
        notify('error', 'Loading', 'Error in fetching data');
      }).complete(function() {
        checkProfile(renderRooms);
      });
      break;
    case ENUMERATE.RUNTIME:
      service.hide();
      room.hide();
      runtime.show();
      title.text("MCU nodes in cluster");
      checkProfile(renderCluster);
      break;
  }
}