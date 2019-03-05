// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

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

var roomTotal = 1;

function checkProfile(callback) {
  var serviceId = getCookie('serviceId') === '' ? top.serviceId : getCookie('serviceId');
  var serviceKey = getCookie('serviceKey') === '' ? top.serviceKey : getCookie('serviceKey');
  top.serviceId = serviceId;
  $('#inputId').val(serviceId);
  $('#inputKey').val(serviceKey);
  if (serviceId === '' || serviceKey === '') {
    $('#myModal').modal('show');
    return;
  }
  nuve = Nuve.init(serviceId, serviceKey);
  nuve.getService(serviceId, function (err, text) {
    if (err) {
      notify('error', 'Failed to get service information', err);
    } else {
      var myService = JSON.parse(text);
      roomTotal = myService.rooms.length;
    }
    judgePermissions();
    callback();
  });
}

$('button#clearCookie').click(function() {
  document.cookie = 'serviceId=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  document.cookie = 'serviceKey=; expires=Thu, 01 Jan 1970 00:00:00 UTC';
  document.getElementById("inputId").value = "";
  document.getElementById("inputKey").value = "";
});

$('button#saveServiceInfo').click(function() {
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
    if (!err) {
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
      title.text("Services");
      checkProfile(renderService);
      break;
    case ENUMERATE.ROOM:
      title.text("Rooms in current Service");
      checkProfile(renderRoom);
      break;
    case ENUMERATE.RUNTIME:
      title.text("MCU nodes in cluster");
      checkProfile(renderCluster);
      break;
  }
}

var login = new Promise((resolve, reject) => {
  $(".modal-backdrop").css("height", document.body.scrollHeight);
  $(".close").on("click", function() {
    if (serviceId === '' || serviceKey === '') {
      return;
    } else {
      $("#myModal").modal("hide");
    }
  });
  checkProfile(()=>resolve());
});
