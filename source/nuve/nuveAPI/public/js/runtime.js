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
    renderCluster();
  });

  var tableTemplate = '{{#nodes}}' + $('div#mcu-ecs-table tbody').html() + '{{/nodes}}';
  Mustache.parse(tableTemplate);
  function tableHandler (nodes) {
    var template = tableTemplate;
    var view = Mustache.render(template, {
      nodes: nodes,
      address: function () {
        if (this.hostname !== '') return this.hostname;
        return this.ip;
      }
    });
    $('div#mcu-ecs-table tbody').html(view);
  }

  var graphicTemplate = '{{#nodes}}<div class="col-xs-6 col-sm-3 placeholder" id="{{rpcID}}">\
    <a href="#"><img class="img-responsive" src="img/node.svg" width="33%"></a>\
    <h4 class="text-{{status}}">{{rpcID}}</h4>\
  </div>{{/nodes}}';
  Mustache.parse(graphicTemplate);
  var configTableTemplate = '<tr>\
      <th>{{defaultVideoBW}}</th>\
      <th>{{maxVideoBW}}</th>\
      <th>{{turn}}</th>\
      <th>{{stun}}</th>\
      <th>{{warning_n_rooms}}</th>\
      <th>{{limit_n_rooms}}</th>\
    </tr>';
  Mustache.parse(configTableTemplate);
  function graphicHandler (nodes) {
    var view = Mustache.render(graphicTemplate, {
      nodes: nodes,
      status: function () {
        if (this.state === 0) return 'danger';
        else if (this.state === 1) return 'warning';
        return 'success';
      }
    });
    $('div#mcu-ecs').html(view);
    var nodeConfHandler = (function () {
      var opened = false;
      return function () {
        if (opened) {
          opened = false;
          return $('div#ec-config-table').hide();
        }
        opened = true;
        var nodeId = $(this).attr('id');
        nuve.getClusterNodeConfig(nodeId, function (err, resp) {
          if (err) return notify('error', 'Get Node Configuration', err);
          var config = JSON.parse(resp);
          config.turn = function () {
            if (typeof this.turnServer === 'object') return JSON.stringify(this.turnServer);
            return ''+this.turnServer;
          };
          config.stun = function () {
            if (typeof this.stunServerUrl === 'object') return JSON.stringify(this.stunServerUrl);
            return ''+this.stunServerUrl;
          };
          $('div#ec-config-table tbody').html(Mustache.render(configTableTemplate, config));
          $('div#ec-config-table #tableTitle').html('Configuration for <em class="text-primary">'+nodeId+'</em>');
          $('div#ec-config-table').show();
        });
      };
    }());
    $('div#mcu-ecs').children().click(nodeConfHandler);
  }

  function renderCluster () {
    nuve.getClusterNodes(function (err, resp) {
      if (err) return notify('error', 'Cluster Info', err);
      var nodes = JSON.parse(resp);
      tableHandler(nodes.map(function (node, index) {
        node.index = index;
        return node;
      }));
      graphicHandler(nodes);
    });
  }

  window.onload = function () {
    checkProfile(renderCluster);
  };

}());
