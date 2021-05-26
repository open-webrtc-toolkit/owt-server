// Copyright (C) <2019> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

'use strict';

var restApi;
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
  restApi = ManagementApi.init();
  restApi.loginCheck(function(err, text) {
    if (err === 401) {
      $('#myModal').modal('show');
      return;
    } else if (err) {
      notify('error', 'Failed to get service information', err);
      return;
    } else {
      var myService = JSON.parse(text);
      roomTotal = myService.rooms.length;
      serviceId = myService._id;
      document.getElementById("inputId").value = serviceId;
      judgePermissions();
      callback();
    }
  });
}

$('button#clearCookie').click(function() {
  restApi = ManagementApi.init();
  restApi.logout(function(err) {
    if (err) {
      notify('error', 'Failed to logout', err);
      return;
    }
  });
  document.getElementById("inputId").value = "";
  document.getElementById("inputKey").value = "";
});

$('button#saveServiceInfo').click(function() {
  serviceId = $('.modal-body #inputId').val();
  serviceKey = $('.modal-body #inputKey').val();
  if (serviceId !== '' && serviceKey !== '') {
    restApi = ManagementApi.init();
    restApi.login(serviceId, serviceKey, function(err) {
      if (err) {
        notify('error', 'Failed to login', err);
        return;
      }
      document.getElementById("inputKey").value = "";
      judgePermissions();
    });
    if (restApi) {
      $("#myModal").modal("hide");
      renderRoom();
    }
  }
});

function judgePermissions(flag) {
  restApi.getServices(function(err, text) {
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
      title.text("MCU runtime");
      checkProfile(renderCluster);
      break;
  }
}

var login = new Promise((resolve, reject) => {
  $(".close").on("click", function() {
    if (serviceId === '' || serviceKey === '') {
      return;
    } else {
      $("#myModal").modal("hide");
    }
  });
  checkProfile(()=>resolve());
});

login.then(()=> {
  renderRoom();
});
