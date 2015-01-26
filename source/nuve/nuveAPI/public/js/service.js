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

  function ServiceCreation () {
    this.toggle = function () {
      $('#myModal2').modal('toggle');
    };
    $('#myModal2 .modal-footer button:first').click(function () {
      var serviceName = $('#cs-form #cs-name').val();
      var serviceKey = $('#cs-form #cs-key').val();
      if (serviceName !== '' && serviceKey !== '') {
        nuve.createService(serviceName, serviceKey, function (err, text) {
          if (err) return notify('error', 'Service Creation', err);
          notify('info', 'Service Creation', text);
          renderServices();
        });
      } else {
        notify('error', 'Service Creation', 'invalid service name or key');
      }
    });
    $('#myModal2').on('hidden.bs.modal', function () {
      $('#cs-form #cs-name').val('');
      $('#cs-form #cs-key').val('');
    });
  }

  var serviceCreation = new ServiceCreation();

  $('img#createService').click(function () {
    // toggle service creation form
    serviceCreation.toggle();
  });

  $('#myModal').on('hidden.bs.modal', function () { // handle profile setting applying
    renderServices();
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
  function tableHandler (rooms, sname) {
    var template = tableTemplate;
    var view = Mustache.render(template, {
      rooms: rooms,
      mediaSetting: function () {
        if (this.mediaMixing === null) return 'default';
        return 'customized';
      }
    });
    $('div#serviceTable tbody').html(view);
    $('h3#tableTitle').html('Rooms in service <em class="text-primary">'+sname+'</em>');
  }

  var graphicTemplate = '{{#services}}<div class="col-xs-6 col-sm-3 placeholder" id="service_{{_id}}">\
    <a href="#"><img class="img-responsive" src="img/service.svg" width="50%"></a>\
    <h4>{{name}}</h4>\
    <code>{{_id}}</code><button type="button" class="close" aria-label="Close"><span aria-hidden="true">&times;</span></button><br />\
    <span>{{rooms.length}} rooms</span>\
  </div>{{/services}}';
  Mustache.parse(graphicTemplate);
  function graphicHandler (services) {
    var view = Mustache.render(graphicTemplate, {
      services: services
    });
    $('div#serviceGraph').html(view);
    services.map(function (service) {
      var id = service._id;
      $('div#service_' + id + ' img').click(function () {
        tableHandler(service.rooms, service.name);
      });
      $('div#service_' + id + ' button.close').click(function () {
        notifyConfirm('Delete Service', 'Are you sure want to delete service ' + id, function () {
          nuve.deleteService(id, function (err, resp) {
            if (err) return notify('error', 'Delete Service Error', resp);
            renderServices();
            notify('info', 'Delete Service Success', resp);
          });
        });
      });
    });
  }

  function renderServices () {
    nuve.getServices(function (err, text) {
      if (err) {return notify('error', 'Get Services', err);}
      try {
        var obj = JSON.parse(text);
        graphicHandler(obj);
      } catch (e) {
        notify('error', 'Service Creation', e);
      }
    });
  }

  window.onload = function () {
    checkProfile(renderServices);
  };

}());
