/*
 * Copyright © 2015 Intel Corporation. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
var lastColor = "rgb(255, 255, 255)";

var roomPage = 1;
var roomPerPage = 50;
var roomTotalPage = 1;

var BrutusinForms = brutusin["json-forms"];
BrutusinForms.onValidationError = function (element, message) {
    element.focus();
    if (!element.className.includes(" error")) {
        element.className += " error";
    }
    console.log(message);
};
var bf = BrutusinForms.create(RoomSchema);

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
  nuve.getService(serviceId, function (err, text) {
    if (err) {
      notify('error', 'Failed to get service information', err);
    } else {
      var myService = JSON.parse(text);
      var total = myService.rooms.length;
      roomTotalPage = Math.ceil(total / roomPerPage);
    }
    judgePermissions();
    initPageSelect();
    callback();
  });
}

$('#selPage').on('change', function() {
  roomPage = this.value;
  renderRooms();
});

function initPageSelect() {
  var i;
  for (i = 1; i <= roomTotalPage; i++) {
    $('#selPage')
         .append($("<option></option>")
                    .attr("value",i)
                    .text(i));
  }
  $('#selPage').val(roomPage);
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
      $(".li").removeClass("hideLi");
    } else {
      $(".li:not(.normal)").addClass("hideLi").removeClass("active");
      $(".li.normal").addClass("active");
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
    $('#serviceModal').modal('toggle');
  };
  $('#serviceModal .modal-footer button:first').click(function() {
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
  $('#serviceModal').on('hidden.bs.modal', function() {
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
      <td>{{DetailSetting}}</td>\
    </tr>{{/rooms}}';
Mustache.parse(tableTemplateService);

function tableHandlerService(rooms, sname) {
  var template = tableTemplateService;
  var view = Mustache.render(template, {
    rooms: rooms,
    DetailSetting: function() {
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
    if (id === serviceId) $('div#service_' + id + ' button.close').hide();
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
      <td class="inputLimit" data-spin="inputLimit">{{inputLimit}}</td>\
      <td class="participantLimit" data-spin="participantLimit">{{participantLimit}}</td>\
      <td class="DetailSetting" data-spin="all">object</td>\
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
      <td class="inputLimit" data-spin="inputLimit"></td>\
      <td class="participantLimit" data-spin="participantLimit"></td>\
      <td class="DetailSetting" data-spin="all"></td>\
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
      var updates = roomCache[roomId];
      unsaved.map(function(index, each) {
        var key = $(each).attr('data-spin');
        var valueObj = $(each).editable('getValue');
        var id = $(each).attr('id');
        updates[key] = valueObj[id];
        if (updates[key] === undefined) {
          // Set undefined to null because undefined will be ignored in JSON.stringfy
          updates[key] = null;
        }
      });
      if (updates.all && typeof updates.all === 'object') {
        updates = updates.all;
      }
      nuve.updateRoom(roomId, updates, function(err, resp) {
        if (err) return notify('error', 'Update Room', resp);
        setTimeout(function() {
          p.find('.editable-unsaved').removeClass('editable-unsaved');
        }, 20);
        roomCache[roomId] = JSON.parse(resp);
        p.find('td.inputLimit').editable('setValue', roomCache[roomId].inputLimit);
        p.find('td.participantLimit').editable('setValue', roomCache[roomId].participantLimit);
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

  var roomEnableMixingHandle = {
    mode: 'inline',
    type: 'select',
    source: [{
      value: 1,
      text: 'true'
    }, {
      value: 0,
      text: 'false'
    }]
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

  var booleanHandle = {
    mode: 'inline',
    type: 'select',
    source: [{
      value: 0,
      text: 'false'
    }, {
      value: 1,
      text: 'true'
    }]
  };

  var textHandle = {
    mode: 'inline',
    validate: function(value) {
      var val = $.trim(value);
      return {
        newValue: val
      };
    }
  };

  var passwordHandle = {
    mode: 'inline',
    type: 'password',
    validate: function(value) {
      var val = $.trim(value);
      return {
        newValue: val
      };
    }
  };

  var ipEditHandle = {
    mode: 'inline',
    validate: function(value) {
      var val = $.trim(value);
      var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
      var hostname = "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$";
      if(!val.match(ipformat) && !val.match(hostname)) return 'This Hostname or IP is not valid';
      return {
        newValue: val
      };
    }
  };

  var deepClone = function(obj) {
      if (typeof obj !== 'object' || obj === null) {
        return obj;
      }
      var result;
      if (Array.isArray(obj)) {
        result = [];
        obj.forEach(function(value) {
          result.push(deepClone(value));
        });
      } else {
        result = {};
        Object.keys(obj).forEach(function(key) {
            result[key] = deepClone(obj[key]);
        });
      }
      return result;
    };

  var viewModal = {create: false, delete: true};
  $('#addview').editable({
      validate: function(value) {
        var reg = /^[a-zA-Z\-0-9]+$/;
        var newLabel = value
        if (!reg.test(newLabel)) {
          return "Input not valid, only a-z0-9- allowed.";
        } else if (viewModal && viewModal.data[newLabel]) {
          return "Label already exist";
        } else {
          viewModal.create = true;
          $("#myTab").append('<li><a href="#"><button class="close closeTab" type="button" >×</button><span>' + newLabel + '</span></a></li>');
          $("#myTab li a span").last().click();
        }
      },
      display: function(value, sourceData) {
        $(this).text('Add New View');
      }
  });

  var mediaSettingFn = function() {
    if (this.parentNode.children[0].innerText === "") return;
    $('#mediaMixingModal').modal('toggle');
    var roomId = $(this).parent().find('td:first').text();
    var room = roomCache[roomId];
    if (typeof room === 'undefined') {
      return notify('error', 'Media Setting', 'error in finding roomId');
    }
    var p = $(this);
    $('#mediaMixingModal .modal-title').text('Media Setting for Room ' + roomId);

    // Setup view tabs
    var views = (deepClone(room.views) || { "common": { mediaMixing: room.mediaMixing } });
    viewModal.data = views;
    var tabContent = {};

    $("#myTab").empty();
    for (var label in views) {
      $("#myTab").append('<li><a href="#"><button class="close closeTab" type="button" >×</button><span>' + label + '</span></a></li>');
    }

    var generateViewTable = function(viewLabel) {
      var videoSetting = null;
      if (!views[viewLabel]) {
        views[viewLabel] = {mediaMixing: {video: {
          resolution: 'vga',
          quality_level: 'standard',
          bkColor: {r: 0, g: 0, b: 0},
          avCoordinated: false,
          multistreaming: 0,
          maxInput: 16,
          crop: false,
          layout: {
            base: 'fluid',
            custom: []
          }
        }}};
      }

      videoSetting = views[viewLabel].mediaMixing.video;

      var viewHtml = Mustache.render('<tr>\
          <td rowspan="9">video</td>\
          <td colspan="2">resolution</td>\
          <td id="resolution" class="value-num-edit" data-value={{resolution}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">quality level</td>\
          <td id="quality_level" class="value-num-edit" data-value={{quality_level}}></td>\
        </tr>\
        <tr>\
          <td colspan="2">bkColor</td>\
          <td id="bkColor" class="value-num-edit"><input id="color" style="border: 0px; outline: 0px;"></td>\
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
          <td colspan="2">crop</td>\
          <td id="crop" class="value-num-edit" data-value={{crop}}></td>\
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

        //videoSetting.bkColor = changeObj2RGB(videoSetting.bkColor);

      return {
        data: videoSetting,
        html: $(viewHtml)
      };
    };

    var unsaveView = {};
    var currentLabel = null;

    var saveCurrentView = function() {
      if (currentLabel && tabContent[currentLabel]) {
        var unsaved = tabContent[currentLabel].html.find('.editable-unsaved');
        if (unsaved.length > 0) {
          tabContent[currentLabel].unsave = true;
          unsaved.map(function(index, each) {
            var id = $(each).attr('id');
            var val = $(each).editable('getValue')[id];
            setVal(tabContent[currentLabel].data, id, val);
          });
        }
      }
    };

    var switchView = function(viewLabel) {
      if (!tabContent[viewLabel]) {
        tabContent[viewLabel] = generateViewTable(viewLabel);
      }

      var viewTable = tabContent[viewLabel];

      $('#mediaMixingModal #inRoomTable tbody').empty().append(viewTable.html);
      enabledMixing(viewTable.data);

      var modalColor = (tabContent[viewLabel].updateColor || viewTable.data.bkColor);
      modalColor = changeRGBA(changeObj2RGB(modalColor));

      $('#mediaMixingModal tbody td#bkColor input#color').ColorPickerSliders({
        color: modalColor,
        flat: false,
        sliders: false,
        swatches: false,
        hsvpanel: true,
        onchange: function(container, color) {
          if (tabContent[viewLabel]) {
            tabContent[viewLabel].updateColor = {r: color.rgba.r, g: color.rgba.g, b: color.rgba.b};
          }
        }
      });
    };

    $("#myTab").off("click", "a");
    $("#myTab").on("click", "a", function (e) {
      e.preventDefault();
      $(this).children("span").first().click();
    });
    $("#myTab").off("click", "a span");
    $("#myTab").on("click", "a span", function (e) {
      e.preventDefault();
      e.stopPropagation();
      $(this).parent().tab('show');
      saveCurrentView();
      currentLabel = $(this).html();
      switchView(currentLabel);
    });
    $("#myTab").off("click", "a button");
    $("#myTab").on("click", "a button", function (e) {
      e.preventDefault();
      var dlabel = $(this).parent().find("span").html();
      if ($("#myTab li a").length === 1) {
        // won't delete
        return;
      }
      //there are multiple elements which has .closeTab icon so close the tab whose close icon is clicked
      if (views[dlabel]) {
        delete views[dlabel];
        delete tabContent[dlabel];
        viewModal.delete = true;
      }
      $(this).parent().parent().remove(); //remove li of tab
      currentLabel = null;
      $("#myTab li a span").first().click();
    });

    // Chose the first
    $("#myTab li a span").first().click();

    $('#mediaMixingModal .modal-footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal"><i class="glyphicon glyphicon-ok"></i></button>\
        <button type="button" class="btn btn-default" data-dismiss="modal"><i class="glyphicon glyphicon-remove"></i></button>');

    // <audio not supported now>
    $('#mediaMixingModal .modal-footer button:first').click(function() {
      var content;
      var needUpdate = false;
      var viewLabel;

      saveCurrentView();
      for (viewLabel in tabContent) {
        if (tabContent[viewLabel].unsave) {
          needUpdate = true;
        }

        if (tabContent[viewLabel].updateColor) {
          needUpdate = true;
          setVal(views[viewLabel].mediaMixing.video, 'bkColor', tabContent[viewLabel].updateColor);
        }

        if (!room.views || !room.views[viewLabel]) {
          needUpdate = true;
        }
      }

      if (!needUpdate && !viewModal.create && !viewModal.delete) return;

      p.addClass('editable-unsaved');
      p.editable('setValue', views);
      p.text('object');
    });
  };

  var enabledMixing = function(videoSetting) {
    $('#mediaMixingModal tbody td#resolution').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.resolution.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });

    $('#mediaMixingModal tbody td#quality_level').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.quality_level.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });

    //$('#mediaMixingModal tbody td#maxInput').editable(numberHandle);
    $('#mediaMixingModal tbody td#maxInput').editable({
      mode: 'inline',
      validate: function(value) {
        var val = parseInt(value, 10);
        if (isNaN(val) || val < 0 || val > 100/*FIXME: hard coded the up border of maxInputto 100*/) return 'value should be a number in [0, 100]';
        return {
          newValue: val
        };
      }
    });
    // $('#mediaMixingModal tbody td#bkColor').editable(numberHandle);
    $('#mediaMixingModal tbody td#avCoordinated').editable(booleanHandle);
    $('#mediaMixingModal tbody td.value-num-edit:last').editable({
      mode: 'inline',
      type: 'select',
      source: metadata.mediaMixing.video.layout.base.map(function(v) {
        return {
          value: v,
          text: v
        };
      })
    });
    $('#mediaMixingModal tbody td#multistreaming').editable(booleanHandle);
    $('#mediaMixingModal tbody td#crop').editable(booleanHandle);
    $('#mediaMixingModal tbody td.value-obj-edit').editable({
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
  }

  var disabledMixing = function() {
    $('#mediaMixingModal tbody td#resolution').editable(disabledHandle);
    $('#mediaMixingModal tbody td#quality').editable(disabledHandle);
    $('#mediaMixingModal tbody td#maxInput').editable(disabledHandle);
    $('#mediaMixingModal tbody td#bkColor').editable(disabledHandle);
    $('#mediaMixingModal tbody td#avCoordinated').editable(disabledHandle);
    $('#mediaMixingModal tbody td#crop').editable(disabledHandle);
    $('#mediaMixingModal tbody td.value-num-edit:last').editable(disabledHandle);
    $('#mediaMixingModal tbody td.value-obj-edit').editable(disabledHandle);
  }

  var audioName2Format = function(v) {
    if (typeof v === 'string') {
      var fields = v.split('_');
      return {
        codec: fields[0],
        sampleRate: Number(fields[1]) || undefined,
        channelNum: Number(fields[2]) || undefined,
      };
    } else {
      return v;
    }
  };
  var audioFormat2Name = function(v) {
    if (v === null) return 'opus';
    if (typeof v === 'object') {
      var str = v.codec;
      if (v.sampleRate) str += '_' + v.sampleRate;
      if (v.channelNum) str += '_' + v.channelNum;
      return str;
    } else {
      return v;
    }
  }

  var videoName2Format = function(v) {
    if (typeof v === 'string') return { codec: v };
    return v;
  };
  var videoFormat2Name = function (v) {
    if (v === null) return 'h264';
    if (typeof v === 'object') return v.codec;
    return v;
  }

  var translateRoomFormat = function (room) {
    if (room.views) {
      room.views.forEach(function(view) {
        view.audio.format = audioFormat2Name(view.audio.format);
        view.video.format = videoFormat2Name(view.video.format);
      });
    }
    if (room.mediaIn) {
      room.mediaIn.audio = room.mediaIn.audio.filter((v) => v).map(audioFormat2Name);
      room.mediaIn.video = room.mediaIn.video.filter((v) => v).map(videoFormat2Name);
    }
    if (room.mediaOut) {
      room.mediaOut.audio = room.mediaOut.audio.filter((v) => v).map(audioFormat2Name);
      room.mediaOut.video.format = room.mediaOut.video.format.filter((v) => v).map(videoFormat2Name);
    }
    return room;
  };

  var untranslateRoomFormat = function (room) {
    if (room.views) {
      room.views.forEach(function(view) {
        if (view.audio) view.audio.format = audioName2Format(view.audio.format);
        if (view.video) view.video.format = videoName2Format(view.video.format);
      });
    }
    if (room.mediaIn) {
      if (room.mediaIn.audio) {
        room.mediaIn.audio = room.mediaIn.audio.filter((v) => v).map(audioName2Format);
      }
      if (room.mediaIn.video) {
        room.mediaIn.video = room.mediaIn.video.filter((v) => v).map(videoName2Format);
      }
    }
    if (room.mediaOut) {
      if (room.mediaOut.audio) {
        room.mediaOut.audio = room.mediaOut.audio.filter((v) => v).map(audioName2Format);
      }
      if (room.mediaOut.video && room.mediaOut.video.format) {
        room.mediaOut.video.format = room.mediaOut.video.format.filter((v) => v).map(videoName2Format);
      }
    }
    return room;
  };

  var editRoomFn = function() {
    if (this.parentNode.children[0].innerText === "") return;
    $('#AllModal').modal('toggle');
    var roomId = $(this).parent().find('td:first').text();
    var room = roomCache[roomId];
    if (typeof room === 'undefined') {
      return notify('error', 'Room Editor', 'error in finding roomId');
    }
    var p = $(this);
    $('#AllModal .modal-title').text('Configuration for Room ' + roomId);

    translateRoomFormat(room);
    var container = document.getElementById('jfContainer');
    while (container.firstChild) {
      container.removeChild(container.firstChild);
    }
    bf.render(container, room);
    untranslateRoomFormat(room);

    $('#AllModal .modal-footer button:first').unbind('click').click(function() {
      if (bf.validate()) {
        p.addClass('editable-unsaved');
        p.editable('setValue', untranslateRoomFormat(bf.getData()));
        p.text('object');
      } else {
        return false;
      }
    });
  };

  var sipConnectivityFn = function() {
    if (this.parentNode.children[0].innerText === "") return;
    $('#SIPModal').modal('toggle');
    var roomId = $(this).parent().find('td:first').text();
    var room = roomCache[roomId];
    if (typeof room === 'undefined') {
      return notify('error', 'SIP Connectivity', 'error in finding roomId');
    }
    var p = $(this);
    $('#SIPModal .modal-title').text('SIP Connectivity for Room ' + roomId);

    var sipSetting = room.sipInfo;
    var sipDefault = {
      sipServer : '127.0.0.1',
      username : '',
      password : ''
    };
    var sipTemplate = '\
      <tr>\
        <td colspan="2">SIP Server</td>\
        <td id="sipServer" class="value-ip-edit SIPinput" data-value={{sipServer}}></td>\
      </tr>\
      <tr>\
        <td colspan="2">User Name</td>\
        <td id="username" class="value-text-edit SIPinput" data-value={{username}}></td>\
      </tr>\
      <tr>\
        <td colspan="2">Password</td>\
        <td id="password" class="value-password-edit SIPinput" data-value={{password}}></td>\
      </tr>';

    var view = Mustache.render(sipTemplate, sipSetting || sipDefault);
    $('#SIPModal #SIPTable tbody').html(view);

    $('#SIPModal #SIPTable tbody td.value-ip-edit').editable(ipEditHandle);
    $('#SIPModal #SIPTable tbody td.value-text-edit').editable(textHandle);
    $('#SIPModal #SIPTable tbody td.value-password-edit').editable(passwordHandle);

    $('#SIPModal .modal-footer').html('<button type="button" class="btn btn-primary" data-dismiss="modal"><i class="glyphicon glyphicon-ok"></i></button>\
        <button type="button" class="btn btn-default" data-dismiss="modal"><i class="glyphicon glyphicon-remove"></i></button>');

    var sipBefore = !!sipSetting;
    if (sipBefore) {
      $('#SIPModal #SIPSwitch').prop('checked', true);
      $('#SIPModal #SIPTable').show();
    } else {
      $('#SIPModal #SIPSwitch').prop('checked', false);
      $('#SIPModal #SIPTable').hide();
    }

    var sipAfter = sipBefore;
    $('#SIPModal #SIPSwitch').click(function() {
      if (this.checked) {
        $('#SIPModal #SIPTable').show();
      } else {
        $('#SIPModal #SIPTable').hide();
      }
      sipAfter = this.checked;
    });

    $('#SIPModal .modal-footer button:first').click(function() {
      var unsaved = $('#SIPModal tbody .editable-unsaved');
      if (sipAfter === sipBefore) {
        if ((sipAfter && unsaved.length === 0) || !sipAfter) {
          return;
        }
      }

      if (sipAfter) {
        sipSetting = {};
        $('#SIPModal .SIPinput').each(function(index) {
          var id = $(this).attr('id');
          var val = $(this).editable('getValue')[id] + '';
          setVal(sipSetting, id, val);
        });
      } else {
        sipSetting = null;
      }

      room.sipInfo = sipSetting;
      p.addClass('editable-unsaved');
      p.editable('setValue', room.sipInfo);
      p.text('object');
    });
  };

  $('td.roomName').editable(roomNameHandle);
  $('td.roomMode').editable(roomModeHandle);
  $('td.inputLimit').editable(numberHandle1);
  $('td.participantLimit').editable(numberHandle1);
  $('td.enableMixing').editable(roomEnableMixingHandle);
  $('td.DetailSetting').editable(disabledHandle);
  $('td.DetailSetting').click(editRoomFn);

  $('#mediaMixingModal').on('hidden.bs.modal', function() {
    $('#mediaMixingModal #inRoomTable tbody').empty();
  });

  $('button#add-room').click(function() {
    var p = $(this).parent().parent();
    if (p.find('td.roomName').hasClass('editable-empty')) {
      return notify('error', 'Add Room', 'Empty room name');
    }
    var roomName = p.find('td.roomName').text();
    var roomMode = p.find('td.roomMode').text() == "Empty" ? undefined : p.find('td.roomMode').text();
    var inputLimit = parseInt(p.find('td.inputLimit').text(), 10);
    var participantLimit = parseInt(p.find('td.participantLimit').text(), 10);
    var room = {
      name: roomName,
      options: {
        inputLimit: inputLimit || -1,
        participantLimit: participantLimit || -1
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
      selector.find('td.inputLimit').editable(numberHandle1);
      selector.find('td.participantLimit').click(numberHandle1);
      selector.find('td.DetailSetting').click(editRoomFn);
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
  nuve.getRooms(roomPage, roomPerPage, function(err, resp) {
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

function changeRGBA2RGB(color) {
  if (color.toLowerCase().indexOf("rgba") > -1) {
    color = color.replace("rgba", "rgb");
  }
  if (color.toLowerCase().indexOf("rgb") > -1) {
    color = color.replace("(", ", ");
    color = color.replace(")", "");
    var tmp = color.split(", ");
    color = {
      r: parseInt(tmp[1]),
      g: parseInt(tmp[2]),
      b: parseInt(tmp[3])
    };
  }
  return color;
}


function changeRGBA(str) {
  switch (str) {
    case "black":
      return "rgb(0, 0, 0)";
    case "white":
      return "rgb(255, 255, 255)";
    default:
      return str;
  }
}

function changeObj2RGB(obj) {
  if (typeof obj === "object")
    return "rgb(" + obj.r + ", " + obj.g + ", " + obj.b + ")";
  else
    return obj;
}
