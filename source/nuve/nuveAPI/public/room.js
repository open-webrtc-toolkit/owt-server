/* global Nuve, Mustache, notify, notifyConfirm, getCookie, setCookie, $ */
(function serviceManagement () {
  'use strict';

  var nuve;

  function checkProfile (callback) {
    var serviceId = getCookie('serviceId');
    var serviceKey = getCookie('serviceKey');
    $('.modal-body #inputId').val(serviceId);
    $('.modal-body #inputKey').val(serviceKey);
    if (serviceId === '' || serviceKey === '') {
      $('button#profile').click();
      return;
    }
    nuve = Nuve.init(serviceId, serviceKey);
    callback();
  }

  $('button#clearCookie').click(function () {
    document.cookie = 'serviceId=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
    document.cookie = 'serviceKey=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  });

  $('button#saveSericeInfo').click(function () {
    var serviceId = $('.modal-body #inputId').val();
    var serviceKey = $('.modal-body #inputKey').val();
    var rememberMe = $('.modal-body .checkbox input').prop('checked');
    if (serviceId !== '' && serviceKey !== '') {
      if (rememberMe) {
        setCookie('serviceId', serviceId, 365);
        setCookie('serviceKey', serviceKey, 365);
      }
      nuve = Nuve.init(serviceId, serviceKey);
    }
  });

  $('#myModal').on('hidden.bs.modal', function () { // handle profile setting applying
    renderRooms();
  });

  var tableTemplate = '{{#rooms}}<tr>\
      <td><code>{{_id}}</code></td>\
      <td id="roomName" data-spin="name">{{name}}</td>\
      <td id="roomMode" data-spin="mode" data-value="{{mode}}"></td>\
      <td id="publishLimit" data-spin="publishLimit">{{publishLimit}}</td>\
      <td id="userLimit" data-spin="userLimit">{{userLimit}}</td>\
      <td id="mediaSetting" data-spin="mediaMixing">{{mediaSetting}}</td>\
      <td class="col-md-1"><button type="button" id="apply-room" class="btn btn-xs btn-primary">Apply</button> <button type="button" id="delete-room" class="btn btn-xs btn-danger">Delete</button></td>\
    </tr>{{/rooms}}';
  Mustache.parse(tableTemplate);
  var modeList = ['Hybrid', 'P2P', 'Forward', 'Mix'];

  function tableHandler (rooms) {
    var template = tableTemplate;
    var view = Mustache.render(template, {
      rooms: rooms,
      mediaSetting: function () {
        if (this.mediaMixing === null) return 'default';
        return 'customized';
      }
    });
    $('div#serviceTable tbody').html(view);
    $('div#serviceTable tbody').append('<tr>\
      <td></td>\
      <td id="roomName" data-spin="name"></td>\
      <td id="roomMode" data-spin="mode"></td>\
      <td id="publishLimit" data-spin="publishLimit"></td>\
      <td id="userLimit" data-spin="userLimit"></td>\
      <td id="mediaSetting" data-spin="mediaMixing"></td>\
      <td class="col-md-1"><button type="button" id="add-room" class="btn btn-xs btn-success">Add</button> <button type="button" id="reset-room" class="btn btn-xs btn-warning">Reset</button></td>\
    </tr>');

    $('button#apply-room').click(function () {
      var p = $(this).parent().parent();
      var unsaved = p.find('.editable-unsaved');
      if (unsaved.length === 0) {
        return notify('warning', 'Update Room', 'nothing to update');
      }
      var roomId = p.find('td:first').text();
      if (roomId !== '') {
        var updates = {};
        unsaved.map(function (index, each) {
          var key = $(each).attr('data-spin');
          var valueObj = $(each).editable('getValue');
          var id = $(each).attr('id');
          updates[key] = valueObj[id];
        });
        nuve.updateRoom(roomId, updates, function (err, resp) {
          if (err) return notify('error', 'Update Room', resp);
          setTimeout(function () {
            p.find('.editable-unsaved').removeClass('editable-unsaved');
          }, 20);
          notify('info', 'Update Room Success', JSON.parse(resp)._id);
        });
      } else {
        notify('error', 'Update Room', 'error in finding roomId');
      }
    });

    var roomNameHandle = {
      mode: 'inline',
      validate: function(value) {
        var val = $.trim(value);
        if (val === '') return 'This field is required';
        return {newValue: val};
      }
    };
    var roomModeHandle = {
      mode: 'inline',
      type: 'select',
      source: modeList.map(function (mode, id) {
        return {value: id, text: mode};
      })
    };
    var publishLimitHandle = {
      mode: 'inline',
      validate: function (value) {
        var val = parseInt(value, 10);
        if (isNaN(val) || val < 0) return 'value should be a number in [0, +Infinity)';
        return {newValue: val};
      }
    };
    var userLimitHandle = {
      mode: 'inline',
      validate: function (value) {
        var val = parseInt(value, 10);
        if (isNaN(val) || val < 0) return 'value should be a number in [0, +Infinity)';
        return {newValue: val};
      }
    };
    $('td#roomName').editable(roomNameHandle);
    $('td#roomMode').editable(roomModeHandle);
    $('td#publishLimit').editable(publishLimitHandle);
    $('td#userLimit').editable(userLimitHandle);

    $('button#add-room').click(function () {
      var p = $(this).parent().parent();
      if (p.find('td#roomName').hasClass('editable-empty')) {
        return notify('error', 'Add Room', 'Empty room name');
      }
      var roomName = p.find('td#roomName').text();
      var roomMode = modeList.indexOf(p.find('td#roomMode').text());
      if (roomMode === -1) roomMode = 0;
      var publishLimit = parseInt(p.find('td#publishLimit').text(), 10);
      if (isNaN(publishLimit)) publishLimit = 16;
      var userLimit = parseInt(p.find('td#userLimit').text(), 10);
      if (isNaN(userLimit)) userLimit = 100;
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
      nuve.createRoom(room, function (err, resp) {
        if (err) return notify('error', 'Add Room', resp);
        var room1 = JSON.parse(resp);
        var view = Mustache.render(tableTemplate, {
          rooms: [room1]
        });
        $(view).insertBefore(p.parent().find('tr:last'));
        var selector = p.parent().find('tr:nth-last-child(2)');
        selector.find('td#roomName').editable(roomNameHandle);
        selector.find('td#roomMode').editable(roomModeHandle);
        selector.find('td#publishLimit').editable(publishLimitHandle);
        selector.find('td#userLimit').editable(userLimitHandle);
        notify('info', 'Add Room Success', room1._id);
      });
    });
    $('button#delete-room').click(function () {
      var p = $(this).parent().parent();
      var roomId = p.find('td:first').text();
      if (roomId !== '') {
        notifyConfirm('Delete Room', 'Are you sure want to delete room ' + roomId, function () {
          nuve.deleteRoom(roomId, function (err, resp) {
            if (err) return notify('error', 'Delete Room', resp);
            p.remove();
            notify('info', 'Delete Room Success', resp);
          });
        });
      } else {
        notify('error', 'Delete Room', 'error in finding roomId');
      }
    });
    $('button#reset-room').click(function () {
      var p = $(this).parent().parent();
      p.find('.editable-unsaved').editable('setValue', null).removeClass('editable-unsaved');
    });
  }

  function renderRooms () {
    nuve.getRooms(function (err, resp) {
      if (err) return notify('error', 'Cluster Info', err);
      var rooms = JSON.parse(resp);
      tableHandler(rooms);
    });
  }

  window.onload = function () {
    checkProfile(renderRooms);
  };

}());
