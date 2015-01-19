/* global Nuve, Mustache, notify, getCookie, setCookie, $ */
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
      <td>{{name}}</td>\
      <td>{{mode}}</td>\
      <td>{{publishLimit}}</td>\
      <td>{{userLimit}}</td>\
      <td>{{mediaSetting}}</td>\
    </tr>{{/rooms}}';
  Mustache.parse(tableTemplate);
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
