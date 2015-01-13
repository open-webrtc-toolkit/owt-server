/* global Nuve, Mustache, PNotify, $ */
function setCookie (cname, cvalue, exdays) {
  'use strict';
  var d = new Date();
  d.setTime(d.getTime() + (exdays*24*60*60*1000));
  var expires = 'expires='+d.toUTCString();
  document.cookie = cname + '=' + cvalue + '; ' + expires;
}

function getCookie (cname) {
  'use strict';
  var name = cname + '=';
  var ca = document.cookie.split(';');
  for(var i=0; i<ca.length; i++) {
    var c = ca[i];
    while (c.charAt(0) == ' ') c = c.substring(1);
    if (c.indexOf(name) === 0) return c.substring(name.length, c.length);
  }
  return '';
}

(function () {
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

  function notify(level, title, message) {
    $(function(){
      new PNotify({
        title: title,
        text: message,
        opacity: 0.8,
        type: level
      });
    });
  }

  function notifyConfirm (title, message, onConfirm, onCancle) {
    (new PNotify({
      title: title,
      text: message,
      icon: 'glyphicon glyphicon-question-sign',
      hide: false,
      confirm: {
        confirm: true
      },
      buttons: {
        closer: false,
        sticker: false
      },
      history: {
        history: false
      }
    })).get().on('pnotify.confirm', onConfirm).on('pnotify.cancel', onCancle);
  }

  $('button#clearCookie').click(function () {
    document.cookie = 'serviceId=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
    document.cookie = 'serviceKey=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  });

  $('button#saveSericeInfo').click(function () {
    var serviceId = $('.modal-body #inputId').val();
    var serviceKey = $('.modal-body #inputKey').val();
    var rememberMe = $('modal-body .checkbox input').prop('checked');
    if (serviceId !== '' && serviceKey !== '') {
      if (rememberMe) {
        setCookie('serviceId', serviceId, 365);
        setCookie('serviceKey', serviceKey, 365);
      }
      nuve = Nuve.init(serviceId, serviceKey);
    }
  });

  function ServiceCreation () {
    this.opened = false;
    this.toggle = function () {
      if (this.opened) {
        this.opened = false;
        return this.hide();
      }
      this.opened = true;
      this.show();
    };
    this.show = function () {
      $('#cs-form').show();
    };
    this.hide = function () {
      $('#cs-form').hide();
    };
    // PNotify.prototype.options.styling = 'bootstrap2';
    $('#cs-form #cs-submit').click(function () {
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
      setTimeout(function () {
        $('#cs-form #cs-name').val('');
        $('#cs-form #cs-key').val('');
        serviceCreation.toggle();
      }, 100);
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

  var tableTemplate = '{{#rooms}}' + $('div#serviceTable tbody').html() + '{{/rooms}}';
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
    $('h3#tableTitle').html('Rooms in service <span class="underline">'+sname+'</span>');
  }

  var svgBlue = 'blue.svg';
  var svgGreen = 'green.svg';
  var graphicTemplate = '{{#services}}<div class="col-xs-6 col-sm-3 placeholder" id="service_{{_id}}">\
    <a href="#"><img class="img-responsive" src="{{graph}}"></a>\
    <h4>{{name}}</h4>\
    <code>{{_id}}</code><button type="button" class="close" aria-label="Close"><span aria-hidden="true">&times;</span></button><br />\
    <span>{{rooms.length}} rooms</span>\
  </div>{{/services}}';
  Mustache.parse(graphicTemplate);
  function graphicHandler (services) {
    var view = Mustache.render(graphicTemplate, {
      services: services,
      graph: function () {
        if (this.name === 'superService') {
          tableHandler(this.rooms, this.name);
          return svgGreen;
        }
        return svgBlue;
      }
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
          }, function () {});
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