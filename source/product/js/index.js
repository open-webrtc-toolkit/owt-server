//var serverAddress = 'http://180.153.223.233:3001/';
//var serverAddress = 'http://10.239.33.28:3001/';
var securedServerAddress = 'https://webrtc.sh.intel.com:3004/';
var unsecuredServerAddress = 'http://webrtc.sh.intel.com:3001/';
var serverAddress = unsecuredServerAddress;
var isSecuredConnection = false;
var localStream = null;
var localScreen = null;
var room = null;
var roomId = null;
var serviceKey = null;
var name = 'Anonymous';
var localId = generateId();
var users = [];
var progressTimeOut = null;
var smallRadius = 60;
var largeRadius = 120;
var isMouseDown = false;
var mouseX = null;
var mouseY = null;
var MODES = {
    GALAXY: 'galaxy',
    MONITOR: 'monitor',
    LECTURE: 'lecture'
};
var mode = MODES.GALAXY;
var SUBSCRIBETYPES = {
  FORWARD: 'forward',
  MIX: 'mix'
};
var subscribeType=SUBSCRIBETYPES.FORWARD;
var isScreenSharing = false;
var isLocalScreenSharing = false;
var remoteScreen = null;
var remoteScreenName = null;
var curTime = 0;
var totalTime = 0;
var isMobile = false;
var token;

/* Rewrite some woogeen functions */
Woogeen.Room=Woogeen.Conference;


function login() {
    setTimeout(function() {
        var inputName = $('#login-input').val();
        if (inputName !== '') {
            name = inputName;
            $('#login-panel').addClass('pulse');

            $('#login-panel').hide();
            $('#container').show();
            initWoogeen();
        }
    }, 400);
    if (isMobile && typeof document.body.webkitRequestFullScreen === 'function') {
        document.body.webkitRequestFullScreen();
    }
}

function toggleLoginSetting() {
    $('#default-login').slideToggle();
    $('#setting-login').slideToggle();
}

function loginDisableVideo() {
    $('#login-resolution').slideUp();
}

function loginEnableVideo() {
    $('#login-resolution').slideDown();
}

function exit() {
    if (confirm('Are you sure to exit?')) {
        window.open('', '_self').close();
    }
}

function initWoogeen() {
    L.Logger.setLogLevel(L.Logger.WARNING);
    var videoWidth = 640;
    var videoHeight = 480;
    room = Woogeen.Room.create({});
    if ($('#login-240').hasClass('selected')) {
        videoWidth = 320;
	videoHeight = 240;
    } else if ($('#login-720').hasClass('selected')) {
        videoWidth = 1280;
	videoHeight = 720;
    }
    if ($('#subscribe-type').val() === 'mixed'){
      subscribeType = SUBSCRIBETYPES.MIX;
      mode = MODES.LECTURE;
    }

    Woogeen.LocalStream.create({
        id: localId,
        audio: true,
        video: $('#login-audio-video').hasClass('selected'),
        videoSize: [videoWidth, videoHeight, videoWidth, videoHeight],
        data: true,
        attributes: {
            id: localId,
            name: name
        }
    },function(err, stream){
      localStream=stream;
      connectRoom();
    });
    users.push({
        name: name,
        id: localId
    });
    $('#profile').text(name);

    createToken(localId, 'presenter', serviceKey, function (status, response) {
        if (status !== 200) {
            Woogeen.Logger.error('createToken failed:', response, status);
            sendIm('Failed to connect to the server, please reload the page to '
                    + 'try again.', 'System');
            return;
        }
        token = response;
        //localStream.addEventListener('access-accepted', connectRoom);
        //localStream.addEventListener('access-denied', function() {
        //    sendIm('Failed to get the access to camera. Please '
        //            + 'accept the access to join a conference.', 'System');
        //});
        //localStream.init();
    });
}

function generateId() {
    return Math.floor((1 + Math.random()) * 0x10000 * 0x10000)
            .toString(16).substring(1);
}

function createToken(userName, role, room, callback) {
    var req = new XMLHttpRequest();
    var url = serverAddress + 'createToken/';
    var body = {username: userName, role: role, room: room};

    req.onreadystatechange = function () {
        if (req.readyState === 4 && typeof callback === 'function') {
            callback(req.status, req.responseText);
        }
    };

    req.open('POST', url, true);
    req.setRequestHeader('Content-Type', 'application/json');
    req.send(JSON.stringify(body));
}

function connectRoom() {
    function subscribeToStreams(streams) {
        for (var index in streams) {
            var stream = streams[index];
            if ((!localStream || localStream.id() !== stream.id())) {
              if ((subscribeType === SUBSCRIBETYPES.MIX && stream.id() == 0)
                  || (subscribeType === SUBSCRIBETYPES.FORWARD && stream.id() != 0))
                room.subscribe(stream, (function(){
                  return function(stream){
                    // append name to users for previously online clients
                    var attr = stream.attributes() || [];
                    var thatId = attr['id'] || stream.id();
                    var thatName = attr['name'] || 'Anonymous';
                    var isScreen = attr['type'] === 'screen';
                    var user = getUserFromId(thatId);
                    if (user === null) {
                        users.push({
                            name: thatName,
                            id: thatId
                        });
                        //sendIm(thatName + ' has joined the room.', 'System');
                    } else if (!user['name']) {
                        user['name'] = thatName;
                        //sendIm(thatName + ' has joined the room.', 'System');
                    } else if (isScreen && !isLocalScreenSharing) {
                        remoteScreen = stream;
                        remoteScreenName = thatName;
                        shareScreenChanged(true, false);
                        sendIm(thatName + ' is sharing screen now.', 'System');
                    }

                    // add video of non-local streams
                    if (localId !== thatId) {
                        room.addEventListener('stream-data', function(event) {
                            if (event.stream && event.msg && thatId !== null) {
                                var user = getUserFromId(thatId);
                                if (user && user['id']) {
                                    sendIm(event.msg, user['id']);
                                }
                            }
                        });
                        addVideo(stream);
                    }
                  }
                })(stream), function(err){
                  L.Logger.error('Subscribe stream failed: '+JSON.stringify(err));
                });
            }
        }
    }

    room.addEventListener('stream-added', function (streamEvent) {
        console.log('stream-added!');
        var streams = [];
        streams.push(streamEvent.stream);
        subscribeToStreams(streams);
    });

    room.addEventListener('stream-removed', function (streamEvent) {
        console.log('stream-removed!');
        // Remove stream from DOM
        var stream = streamEvent.stream;
        var attr = stream.attributes() || [];
        var uid = attr['id'] || stream.id();
        var user = getUserFromId(uid);
        if (user !== null && user['htmlId'] !== undefined) {
            if (attr['type'] === 'screen') {
                if (isLocalScreenSharing) {
                    $('#local-screen').addClass('pluse');
                    localScreen.close();
                    localScreen = null;
                    sendIm('You have stopped screen sharing.');
                } else {
                    $('#screen').addClass('pluse');
                    remoteScreen.close();
                    remoteScreen = null;
                    sendIm(attr['name'] + ' has stopped screen sharing.',
                            'System');
                }
            } else {
                $('#client-' + user['htmlId']).addClass('pulse');
            }
            setTimeout(function() {
                if (attr['type'] === 'screen') {
                    if (isLocalScreenSharing) {
                        $('#local-screen').remove();
                    } else {
                        $('#screen').remove();
                    }
                    shareScreenChanged(false, false);
                    changeMode(MODES.LECTURE, $('#client-' + user['htmlId']));
                } else {
                    $('#client-' + user['htmlId']).remove();
                }
                changeMode(mode);
            }, 800);
        }
    });

    room.addEventListener('stream-changed', function (e) {
        console.log('stream '+e.stream.getID()+' has changed');
    });

    room.addEventListener('client-joined', function (e) {
        if (e.user.name !== 'user' && getUserFromId(e.user.name) === null) {
            // new user
            users.push({
                id: e.user.name
            });
        }
    });

    room.addEventListener('client-left', function (e) {
        var user = getUserFromId(e.user.name);
        if (user !== null && user.name !== undefined) {
            sendIm(user['name'] + ' has left the room.', 'System');
        } else {
            sendIm('Anonymous has left the room.', 'System');
        }
    });

    room.join(token,function(res){

      sendIm('Connected to the room.', 'System');
      $('#text-send,#send-btn').show();

      var bandWidth = 100;
      if ($('#login-480').hasClass('selected')) {
          bandWidth = 300;
      } else if ($('#login-720').hasClass('selected')) {
          bandWidth = 1000;
      }

      if (!$('#login-audio-video').hasClass('selected')) {
        room.publish(localStream, {}, function(resp){
          L.Logger.info('Publish stream success.');
        },function(err){
          if(err){
            return L.Logger.error('publish failed:', err);
          }
        });
      } else {
        room.publish(localStream, {maxVideoBW: 500}, function(resp){
          L.Logger.info('Publish stream success.', resp.id());
        },function(err){
          L.Logger.error('publish failed:', err);
        });
      }

      var streams = res.streams;
      subscribeToStreams(streams);
    }, function(err){
      L.Logger.error('Join room failed.');
    });

    addVideo(localStream, true);
}

function shareScreen() {
    if ($('#screen-btn').hasClass('disabled')) {
        return;
    }

    localScreen = Woogeen.Stream({
        screen: true,
        video: true,
        videoSize: [1920, 1080, 1920, 1080],
        attributes: {
            type: 'screen',
            name: name,
            id: localId
        }
    });

    localScreen.addEventListener('access-accepted', function(e) {
        room.publish(localScreen);
        sendIm('You are sharing screen now.');
        $('#video-panel').append('<div id="local-screen" class="client clt-0"' +
                '>Screen Sharing</div>').addClass('screen');
        changeMode(MODES.LECTURE, $('#local-screen'));

        localScreen.stream.addEventListener('ended', function(e) {
            room.unpublish(localScreen);
            console.log('unpublish');
        });
    });

    localScreen.addEventListener('access-denied', function(e) {
        sendIm('Failed to get the access to local media. '
                + 'Please try again with HTTPS and accept it.');
        shareScreenChanged(false, false);
    });

    localScreen.init();
    shareScreenChanged(true, true);
}

// update screen btn when sharing
function shareScreenChanged(ifToShare, ifToLocalShare) {
    isScreenSharing = ifToShare;
    isLocalScreenSharing = ifToLocalShare;
    $('#screen-btn').removeClass('disabled selected');
    if (ifToShare) {
        if (ifToLocalShare) {
            $('#screen-btn').addClass('selected disabled');
        } else {
            $('#screen-btn').addClass('disabled');
        }
        $('#galaxy-btn,#monitor-btn').addClass('disabled');
    } else {
        $('#galaxy-btn,#monitor-btn').removeClass('disabled');
        $('#video-panel').removeClass('screen');
    }
}

// decide next size according to previous sizes and window w and h
function getNextSize() {
    var lSum = $('#video-panel .small').length * smallRadius * smallRadius * 5;
    var sSum = $('#video-panel .large').length * largeRadius * largeRadius * 10;
    var largeP = 1 / ((lSum + sSum) / $('#video-panel').width()
            / $('#video-panel').height() + 1) - 0.5;
    if (Math.random() < largeP) {
        return 'large';
    } else {
        return 'small';
    }
}

function addVideo(stream, isLocal) {
    // compute next html id
    var id = $('#video-panel').children('.client').length;
    while ($('#client-' + id).length > 0) {
        ++id;
    }

    var attr = stream.attributes() || {};
    var uid = attr.id || stream.id();

    // check if is screen sharing
    if (attr['type'] === 'screen') {
        $('#video-panel').addClass('screen')
                .append('<div class="client" id="screen"></div>');
        stream.show('screen');
        $('#screen').addClass('clt-' + getColorId(uid))
                .children().children('div').remove();
        $('#screen').append('<div class="ctrl"><a href="#" class="ctrl-btn '
                + 'fullscreen"></a></div>').append('<div class="ctrl-name">'
                + 'Screen Sharing from ' + attr['name'] + '</div>');

        changeMode(MODES.LECTURE, $('#screen'));

    } else {
        // append to global users
        var thisUser = getUserFromId(uid);
        var htmlClass = isLocal ? 0 : (id - 1) % 5 + 1;
        thisUser.htmlId = id;
        thisUser.htmlClass = thisUser.htmlClass || htmlClass;
        thisUser.stream = stream;
        thisUser.id = uid;

        // append new video to video panel
        var size = getNextSize();
        $('#video-panel').append('<div class="' + size + ' clt-'
                + htmlClass + ' client pulse" ' + 'id="client-'+ id + '"></div>');
        if (isLocal) {
            var opt = {muted: 'muted'};
            $('#client-' + id).append('<div class="self-arrow"></div>');
        }
        stream.show('client-' + id, opt);
        var player = $('#client-' + id).children(':not(.self-arrow)');
        player.attr('id', 'player-' + id).addClass('player')
                .css('background-color', 'inherit');
        player.children('div').remove();
        player.children('video').attr('id', 'video-' + id).addClass('video');

        // add avatar for no video users
        if (stream.video === false) {
            player.parent().addClass('novideo');
            player.append('<img src="img/avatar.png" class="img-novideo" />');
        }

        if(stream.id()==0)
          attr['name'] = "Mixed Stream";

        // control buttons and user name panel
        var resize = size === 'large' ? 'shrink' : 'enlarge';
        var attr = stream.attributes() ? stream.attributes() : [];
        var name = attr['name'] ? attr['name'] : 'Anonymous';
        var muteBtn = '<a href="#" class="ctrl-btn unmute"></a>';
        $('#client-' + id).append('<div class="ctrl">'
                + '<a href="#" class="ctrl-btn ' + resize + '"></a>'
                + '<a href="#" class="ctrl-btn fullscreen"></a>'
                + muteBtn + '</div>')
                .append('<div class="ctrl-name">' + name + '</div>');
        relocate($('#client-' + id));

        if(stream.id()==0){
          name = 'Mixed';
          changeMode(MODES.LECTURE, $('#client-'+id));
        }
        else{
          changeMode(mode);
        }
    }

    function mouseout(e) {
        isMouseDown = false;
        mouseX = null;
        mouseY = null;
        $(this).css('transition', '0.5s');
    }
    // no animation when dragging
    $('.client').mousedown(function(e) {
        isMouseDown = true;
        mouseX = e.clientX;
        mouseY = e.clientY;
        $(this).css('transition', '0s');
    }).mouseup(mouseout).mouseout(mouseout)
    .mousemove(function(e) {
        e.preventDefault();
        if (!isMouseDown || mouseX === null || mouseY === null
                || mode !== MODES.GALAXY) {
            return;
        }
        // update position to prevent from moving outside of video-panel
        var left = parseInt($(this).css('left')) + e.clientX - mouseX;
        var border = parseInt($(this).css('border-width')) * 2;
        var maxLeft = $('#video-panel').width() - $(this).width() - border;
        if (left < 0) {
            left = 0;
        } else if (left > maxLeft) {
            left = maxLeft;
        }
        $(this).css('left', left);

        var top = parseInt($(this).css('top')) + e.clientY - mouseY;
        var maxTop = $('#video-panel').height() - $(this).height() - border;
        if (top < 0) {
            top = 0;
        } else if (top > maxTop) {
            top = maxTop;
        }
        $(this).css('top', top);

        // update data for later calculation position
        $(this).data({
            left: left,
            top: top
        });
        mouseX = e.clientX;
        mouseY = e.clientY;
    });

    // stop pulse when animation completes
    setTimeout(function() {
        $('#client-' + id).removeClass('pulse');
    }, 800);
}

function toggleIm(isToShow) {
    if (isToShow || $('#text-panel').is(":visible")) {
        $('#video-panel').css('left', 0);
    } else {
        $('#video-panel').css('left', $('#text-panel').width());
    }
    $('#text-panel').toggle();
    if ($('#im-btn').hasClass('selected')) {
        $('#im-btn').removeClass('selected');
    } else {
        $('#im-btn').addClass('selected');
    }
    changeMode(mode);
}

function relocate(element) {
    var max_loop = 1000;
    var margin = 20;
    for (var loop = 0; loop < max_loop; ++loop) {
        var r = element.hasClass('large') ? largeRadius : smallRadius;
        var w = $('#video-panel').width() - 2 * r - 2 * margin;
        var y = $('#video-panel').height() - 2 * r - 2 * margin;
        var x = Math.ceil(Math.random() * w + r + margin);
        var y = Math.ceil(Math.random() * y + r + margin);
        var others = $('.client');
        var len = others.length;
        var isFeasible = x + r < $('#video-panel').width() && y + r
                < $('#video-panel').height();
        for (var i = 0; i < len && isFeasible; ++ i) {
            var o_r = $(others[i]).hasClass('large') ? largeRadius : smallRadius;
            var o_x = parseInt($(others[i]).data('left')) + o_r;
            var o_y = parseInt($(others[i]).data('top')) + o_r;
            if ((o_x - x) * (o_x - x) + (o_y - y) * (o_y - y) <
                    (o_r + r + margin) * (o_r + r + margin)) {
                // conflict
                isFeasible = false;
                break;
            }
        }
        if (isFeasible) {
            var pos = {'left': x - r, 'top': y - r};
            element.css(pos).data('left', x - r).data('top', y - r);
            return true;
        }
    }
    // no solution
    var pos = {'left': x - r, 'top': y - r};
    element.css(pos).data('left', x - r).data('top', y - r);
    return false;
}

function sendIm(msg, sender) {
    var time = new Date();
    var hour = time.getHours();
    hour = hour > 9 ? hour.toString() : '0' + hour.toString();
    var mini = time.getMinutes();
    mini = mini > 9 ? mini.toString() : '0' + mini.toString();
    var sec = time.getSeconds();
    sec = sec > 9 ? sec.toString() : '0' + sec.toString();
    var timeStr = hour + ':' + mini + ':' + sec;
    if (msg === undefined) {
        // send local msg
        if ($('#text-send').val()) {
            msg = $('#text-send').val();
            $('#text-send').val('').height('18px');
            $('#text-content').css('bottom', '30px');
            sender = localId;
            // send to server
            if (users[0] && users[0].stream) {
                users[0].stream.sendData(msg);
            }
        } else {
            return;
        }
    }
    var color = getColor(sender);
    var user = getUserFromId(sender);
    var name = user ? user['name'] : 'System';
    $('<p class="' + color + '">').html(timeStr + ' ' + name + '<br />')
            .append(document.createTextNode(msg)).appendTo('#text-content');
    // scroll to bottom of text content
    $('#text-content').scrollTop($('#text-content').prop('scrollHeight'));
}

function updateProgress() {
    if (totalTime === 0) {
        return;
    }
    var width = parseInt(curTime / totalTime * 100);
    if (width >= 100) {
        clearTimeout(progressTimeOut);
        width = 100;
    } else {
        progressTimeOut = setTimeout(updateProgress, 1000);
    }
    curTime += 1;
    $('#progress').animate({
        'width': width + '%'
    }, 500);
}

function getColorId(id) {
    var user = getUserFromId(id);
    if (user) {
        return user.htmlClass;
    } else {
        // screen stream comes earlier than video stream
        var htmlClass = users.length % 5 + 1;
        users.push({
            name: name,
            htmlClass: htmlClass
        });
        return htmlClass;
    }
}

function getColor(id) {
    var user = getUserFromId(id);
    if (user) {
        return 'clr-clt-' + user.htmlClass;
    } else {
        return 'clr-sys';
    }
}

function getUserFromName(name) {
    for (var i = 0; i < users.length; ++i) {
        if (users[i] && users[i].name === name) {
            return users[i];
        }
    }
    return null;
}

function getUserFromId(id) {
    for (var i = 0; i < users.length; ++i) {
        if (users[i] && users[i].id === id) {
            return users[i];
        }
    }
    return null;
}

function toggleMute(id, toMute) {
    for (var i = 0; i < users.length; ++i) {
        if (users[i] && users[i].htmlId === id) {
            users[i].stream.stream.getAudioTracks()[0].enabled = !toMute;
            break;
        }
    }
}

function getColumns() {
    var col = 1;
    var cnt = $('#video-panel video').length;
    if (mode === MODES.LECTURE && !isScreenSharing) {
        --cnt;
    }
    if (cnt === 0) {
        return 0;
    }
    while (true) {
        var width = mode === MODES.MONITOR ?
                Math.floor($('#video-panel').width() / col) :
                Math.floor($('#text-panel').width() / col);
        var height = Math.floor(width * 3 / 4);
        var row = Math.floor($('#video-panel').height() / height);
        if (row * col >= cnt) {
            return col;
        }
        ++col;
    }
}

function changeMode(newMode, enlargeElement) {
    switch(newMode) {
        case MODES.GALAXY:
            if ($('#galaxy-btn').hasClass('disabled')) {
                return;
            }
            mode = MODES.GALAXY;
            $('#monitor-btn,#lecture-btn').removeClass('selected');
            $('#galaxy-btn').addClass('selected');
            $('#video-panel').removeClass('monitor lecture')
                    .addClass('galaxy');
            $.each($('.client'), function(key, value) {
                var d = smallRadius * 2;
                if ($(this).hasClass('large')) {
                    d = largeRadius * 2;
                    $(this).find('.enlarge')
                            .removeClass('enlarge').addClass('shrink');
                } else {
                    $(this).find('.shrink')
                            .removeClass('shrink').addClass('enlarge');
                }
                var left = parseInt($(this).data('left'));
                if (left < 0) {
                    left = 0;
                } else if (left > $('#video-panel').width() - d) {
                    left =  $('#video-panel') - d;
                }
                var top = parseInt($(this).data('top'));
                if (top < 0) {
                    top = 0;
                } else if (top > $('#video-panel').height() - d) {
                    top = $('#video-panel').height() - d;
                }
                $(this).css({
                    left: left,
                    top: top,
                    width: d + 'px',
                    height: d + 'px'
                }).data({
                    left: left,
                    top: top
                });
            });
            break;

        case MODES.MONITOR:
            if ($('#monitor-btn').hasClass('disabled')) {
                return;
            }
            mode = MODES.MONITOR;
            $('#galaxy-btn,#lecture-btn').removeClass('selected');
            $('#monitor-btn').addClass('selected');
            $('#video-panel').removeClass('galaxy lecture')
                    .addClass('monitor');
            $('.shrink').removeClass('shrink').addClass('enlarge');
            updateMonitor();
            break;

        case MODES.LECTURE:
            if ($('#lecture-btn').hasClass('disabled')) {
                return;
            }
            mode = MODES.LECTURE;
            $('#galaxy-btn,#monitor-btn').removeClass('selected');
            $('#lecture-btn').addClass('selected');
            $('#video-panel').removeClass('galaxy monitor')
                    .addClass('lecture');
            $('.shrink').removeClass('shrink').addClass('enlarge');

            var largest = enlargeElement || ($('#screen').length > 0
                    ? $('#screen') : ($('.largest').length > 0
                    ? $('.largest').first() : ($('.large').length > 0
                    ? $('.large').first() : $('.client').first())));
            $('.client').removeClass('largest');
            largest.addClass('largest')
                    .find('.enlarge').removeClass('enlarge').addClass('shrink');
            updateLecture();
            break;

        default:
            console.error('Illegal mode name');
    }
    // update canvas size in all video panels
    $('.player').trigger('resizeVideo');
}

function updateMonitor() {
    var col = getColumns();
    if (col > 0) {
        $('.client').css({
            width: Math.floor($('#video-panel').width() / col),
            height: Math.floor($('#video-panel').width() / col * 3 / 4),
            top: 'auto',
            left: 'auto'
        });
    }
}

function updateLecture() {
    $('.largest,.largest-mask').css({
        width: $('#video-panel').width() - $('#text-panel').width(),
        height: $('#video-panel').height()
    });

    var col = getColumns();
    $('.client').not('.largest').css({
        width: Math.floor($('#text-panel').width() / col),
        height: Math.floor($('#text-panel').width() / col * 3 / 4),
        top: 'auto',
        left: 'auto'
    });
}

function fullScreen(isToFullScreen, element) {
    if (isToFullScreen) {
        element.addClass('full-screen');
    } else {
        element.removeClass('full-screen');
    }
}

function exitFullScreen(ctrlElement) {
    if (ctrlElement.parent().hasClass('full-screen')) {
        fullScreen(false, ctrlElement.parent());
        ctrlElement.append('<a href="#" class="ctrl-btn fullscreen">');
        if (ctrlElement.parent().hasClass('small')
                || mode !== MODES.GALAXY) {
            ctrlElement.children('shrink')
                    .removeClass('shrink').addClass('enlarge');
        }
        return;
    }
    switch(mode) {
        case MODES.GALAXY:
            ctrlElement.children('.shrink')
                    .addClass('enlarge').removeClass('shrink').parent()
                    .parent().removeClass('large').addClass('small');
            break;

        case MODES.MONITOR:
        case MODES.LECTURE:
            changeMode(MODES.LECTURE);
            break;
    }
}

$(document).ready(function() {
    $('.buttonset>.button').click(function() {
        $(this).siblings('.button').removeClass('selected');
        $(this).addClass('selected');
    })

    // name
    if (window.location.search.slice(0, 6) === '?room=') {
        roomId = window.location.search.slice(6);
    }

    if (window.location.protocol === "https:") {
        isSecuredConnection = true;
        serverAddress = securedServerAddress;
    } else {
        isSecuredConnection = false;
        serverAddress = unsecuredServerAddress;
    }

    $(document).on('click', '.shrink', function() {
        exitFullScreen($(this).parent());
        $(this).parent().parent().children('.player').trigger('resizeVideo');
    });

    $(document).on('click', '.enlarge', function() {
        switch(mode) {
            case MODES.GALAXY:
                $(this).addClass('shrink').removeClass('enlarge').parent()
                        .parent().removeClass('small').addClass('large');
            break;

            case MODES.MONITOR:
            case MODES.LECTURE:
                changeMode(MODES.LECTURE, $(this).parent().parent());
                break;
        }
        $(this).parent().parent().children('.player').trigger('resizeVideo');
    });

    $(document).on('click', '.mute', function() {
        // unmute
        var id = parseInt($(this).parent().parent().attr('id').slice(7));
        toggleMute(id, false);
        $(this).addClass('unmute').removeClass('mute');
    });

    $(document).on('click', '.unmute', function() {
        // mute
        var id = parseInt($(this).parent().parent().attr('id').slice(7));
        toggleMute(id, true);
        $(this).addClass('mute').removeClass('unmute');
    });

    $(document).on('click', '.fullscreen', function() {
        fullScreen(true, $(this).parent().parent());
        var enlarge = $(this).siblings('.enlarge');
        if (enlarge.length > 0) {
            enlarge.removeClass('enlarge').addClass('shrink');
        } else if ($(this).siblings('.shrink').length === 0) {
            $(this).parent().append('<a href="#" class="ctrl-btn shrink"></a>');
        }
        $(this).remove();
    });

    $(document).keyup(function(event) {
        if (event.keyCode === 27 && $('.full-screen').length > 0) {
            console.log('full');
            // exit full screen when escape key pressed
            exitFullScreen($('.full-screen .ctrl'));
        }
    });

    $('#text-send').keypress(function(event) {
        if ($(this)[0].scrollHeight > $(this)[0].clientHeight) {
            $(this).height($(this)[0].scrollHeight);
            $('#text-content').css('bottom', $(this)[0].scrollHeight + 'px');
        }
        if (event.keyCode === 13) {
            event.preventDefault();
            // send msg when press enter
            sendIm();
        }
    });

    $('#login-input').keypress(function(event) {
        if (event.keyCode === 13) {
            event.preventDefault();
            login();
        }
    });

    $.get('config.json', function(data) {
        if (data['rooms'] === undefined) {
            alert('No room reserved!');
            return;
        }
        for (var i = 0; i < data['rooms'].length; ++i) {
            if (data['rooms'][i]['id'] === roomId) {
                if (data['rooms'][i]['key']) {
                    serviceKey = data['rooms'][i]['key'];
                } else {
                    serviceKey = 'forward';
                }
                $('#titleText,#login-panel>h3').text(data['rooms'][i]['title']);
                if (data['rooms'][i]['date'] && data['rooms'][i]['start-time']
                        && data['rooms'][i]['end-time']) {
                    $('#titleTime,.time').text(data['rooms'][i]['date'] +
                           ' from ' + data['rooms'][i]['start-time'] + ' to '
                            + data['rooms'][i]['end-time']);
                    var s = new Date(data['rooms'][i]['date'] + ' '
                            + data['rooms'][i]['start-time']);
                    var e = new Date(data['rooms'][i]['date'] + ' '
                            + data['rooms'][i]['end-time']);
                    var now = new Date();
                    totalTime = e - s <= 0 ? 0 : (e - s) / 1000;
                    if (now - s <= 0) {
                        curTime = 0;
                    } else if (now - e >= 0) {
                        curTime = totalTime;
                    } else {
                        curTime = (now - s) / 1000;
                    }
                    updateProgress();
                }
                $('#login-panel').fadeIn();
                return;
            }
        }
        // no roomId founded
        alert('Illegal room id!');
    }).fail(function(e) {
        console.log(e);
        alert('Fail to load the JSON file. See press F12 to open console for more information and report to webtrc_support@intel.com.');
    });

    $(window).resize(function() {
        console.log('resized');
        changeMode(mode);
    });

    checkMobile();
});

function checkMobile() {
    if ((/iphone|ipod|android|ie|blackberry|fennec/).test
            (navigator.userAgent.toLowerCase())) {
        isMobile = true;

        toggleIm(true);
        changeMode(MODES.MONITOR);
        $('#galaxy-btn, .button-split, #screen-btn').hide();
    }
}

