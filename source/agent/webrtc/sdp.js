/*global require, exports*/
'use strict';

const transform = require('sdp-transform');

const h264ProfileOrder = ['E', 'H', 'M', 'B', 'CB'];

function translateProfile (profLevId) {
    var profile_idc = profLevId.substr(0, 2);
    var profile_iop = parseInt(profLevId.substr(2, 2), 16);
    var profile;
    switch (profile_idc) {
        case '42':
            if (profile_iop & (1 << 6)) {
                // x1xx0000
                profile = 'CB';
            } else {
                // x0xx0000
                profile = 'B';
            }
            break;
        case '4D':
            if (profile_iop && (1 << 7)) {
                // 1xxx0000
                profile = 'CB';
            } else if (!(profile_iop && (1 << 5))) {
                profile = 'M';
            }
            break;
        case '58':
            if (profile_iop && (1 << 7)) {
                if (profile_iop && (1 << 6)) {
                    profile = 'CB';
                } else {
                    profile = 'B';
                }
            } else if (!(profile_iop && (1 << 6))) {
                profile = 'E';
            }
            break;
        case '64':
            (profile_iop === 0) && (profile = 'H');
            break;
        case '6E':
            (profile_iop === 0) && (profile = 'H10');
            (profile_iop === 16) && (profile = 'H10I');
            break;
        case '7A':
            (profile_iop === 0) && (profile = 'H42');
            (profile_iop === 16) && (profile = 'H42I');
            break;
        case 'F4':
            (profile_iop === 0) && (profile = 'H44');
            (profile_iop === 16) && (profile = 'H44I');
            break;
        case '2C':
            (profile_iop === 16) && (profile = 'C44I');
            break;
        default:
            break;
    }
    return profile;
}

function filterH264Payload(sdpObj, formatPreference, direction) {
  var removePayloads = [];
  var selectedPayload = -1;
  var tmpProfile = null;
  var preferred = null;
  var optionals = [];
  var finalProfile = null;

  if (formatPreference && formatPreference.video) {
      preferred = formatPreference.video.preferred;
      optionals = formatPreference.video.optional || [];
  }
  sdpObj.media.forEach((mediaInfo) => {
      var i, rtp, fmtp;
      if (mediaInfo.type === 'video') {
          for (i = 0; i < mediaInfo.rtp.length; i++) {
              rtp = mediaInfo.rtp[i];
              if (rtp.codec.toLowerCase() === 'h264') {
                  removePayloads.push(rtp.payload);
              }
          }

          for (i = 0; i < mediaInfo.fmtp.length; i++) {
              fmtp = mediaInfo.fmtp[i];
              if (removePayloads.indexOf(fmtp.payload) > -1) {
                  if (fmtp.config.indexOf('packetization-mode=0') > -1)
                      continue;

                  if (fmtp.config.indexOf('profile-level-id') >= 0) {
                      var params = transform.parseParams(fmtp.config);
                      var prf = translateProfile(params['profile-level-id'].toString());
                      if (preferred && preferred.codec === 'h264') {
                          if (preferred.profile && preferred.profile === prf) {
                              // Use preferred profile
                              selectedPayload = fmtp.payload;
                              finalProfile = prf;
                              break;
                          }
                      }
                      if (direction === 'out') {
                          if (optionals) {
                              if (optionals.findIndex((fmt) => (fmt.codec === 'h264' && fmt.profile === prf)) > -1) {
                                  if (h264ProfileOrder.indexOf(tmpProfile) < h264ProfileOrder.indexOf(prf)) {
                                      tmpProfile = prf;
                                      selectedPayload = fmtp.payload;
                                  }
                              }
                          }
                      } else {
                          if (h264ProfileOrder.indexOf(tmpProfile) < h264ProfileOrder.indexOf(prf)) {
                              tmpProfile = prf;
                              selectedPayload = fmtp.payload;
                          }
                      }
                  }
              }
          }

          if (selectedPayload > -1) {
              i = removePayloads.indexOf(selectedPayload);
              removePayloads.splice(i, 1);
          }

          // Remove non-selected h264 payload
          mediaInfo.rtp = mediaInfo.rtp.filter((rtp) => {
              return removePayloads.findIndex((pl) => (pl === rtp.payload)) === -1;
          });
          mediaInfo.fmtp = mediaInfo.fmtp.filter((fmtp) => {
              return removePayloads.findIndex((pl) => (pl === fmtp.payload)) === -1;
          });
      }
  });

  finalProfile = (finalProfile || tmpProfile);
  return finalProfile;
}

function determineVideo(videoPreference = {}) {

}

function determineAudio(audioPreference = {}) {

}

var isReserve = function (line, reserved) {
  var re = new RegExp('^a=((rtpmap)|(fmtp)|(rtcp-fb))', 'i'),
      re1 = new RegExp('^a=((rtpmap)|(fmtp)|(rtcp-fb)):(' + reserved.map(function(c){return '('+c+')';}).join('|') + ')\\s+', 'i');

  return !re.test(line) || re1.test(line);
};

var isAudioFmtEqual = function (fmt1, fmt2) {
  return (fmt1.codec === fmt2.codec)
    && (((fmt1.sampleRate === fmt2.sampleRate) && ((fmt1.channelNum === fmt2.channelNum) || (fmt1.channelNum === 1 && fmt2.channelNum === undefined) || (fmt1.channelNum === undefined && fmt2.channelNum === 1)))
        || (fmt1.codec === 'pcmu') || (fmt1.codec === 'pcma'));
};

var audio_fmt, video_fmt;
var determineAudioFmt = function (sdp, formatPreference) {
    var lines = sdp.split('\n');
    var a_begin = lines.findIndex(function (line) {return line.startsWith('m=audio');});
    if ((a_begin >= 0) && formatPreference.audio) {
        var a_end = lines.slice(a_begin + 1).findIndex(function(line) {return line.startsWith('m=');});
            a_end = (a_end > 0 ? a_begin + a_end + 1 : lines.length);

        var a_lines = lines.slice(a_begin, a_end);

        var fmtcode = '';
        var fmts = [];
        var fmtcodes = a_lines[0].split(' ').slice(3);
        for (var i in fmtcodes) {
            var filter_lines = a_lines.filter(function (line) {
                return line.startsWith('a=rtpmap:'+fmtcodes[i]);
            });

            if (filter_lines.length === 0) continue;
            var fmtname = filter_lines[0].split(' ')[1].split('/');

            var fmt = {codec: fmtname[0].toLowerCase()};
            (fmt.codec !== 'pcmu' && fmt.codec !== 'pcma') && fmtname[1] && (fmt.sampleRate = Number(fmtname[1]));
            (fmt.codec !== 'pcmu' && fmt.codec !== 'pcma') && fmtname[2] && (fmt.channelNum = Number(fmtname[2]));
            (fmt.codec === 'g722') && (fmt.sampleRate = 16000) && (fmt.channelNum = fmt.channelNum || 1);
            (fmt.codec === 'ilbc') && (delete fmt.sampleRate);

            fmts.push({code: fmtcodes[i], fmt: fmt});

            if ((fmtcode === '') && formatPreference.audio.preferred && isAudioFmtEqual(formatPreference.audio.preferred, fmt)) {
                audio_fmt = fmt;
                fmtcode = fmtcodes[i];
            }
        }

        if ((fmtcode === '') && formatPreference.audio.optional) {
            for (var j in fmts) {
                if (formatPreference.audio.optional.findIndex(function (o) {return isAudioFmtEqual(o, fmts[j].fmt);}) !== -1) {
                    audio_fmt = fmts[j].fmt;
                    fmtcode = fmts[j].code;
                    break;
                }
            }
        }

        if (fmtcode) {
            var reserved_codes = [fmtcode];
            fmts.forEach(function (f) {
                if ((f.fmt.codec === 'telephone-event')
                    || ((f.fmt.codec === 'cn') && f.fmt.sampleRate && (f.fmt.sampleRate === audio_fmt.sampleRate))) {
                    reserved_codes.push(f.code);
                }
            });

            a_lines[0] = lines[a_begin].split(' ').slice(0, 3).concat(reserved_codes).join(' ');
            a_lines = a_lines.filter(function(line) {
                return isReserve(line, reserved_codes);
            }).join('\n');
            return lines.slice(0, a_begin).concat(a_lines).concat(lines.slice(a_end)).join('\n');
        } else {
            log.info('No proper audio format');
            return sdp;
        }
    }
    return sdp;
};

var isVideoFmtEqual = function (fmt1, fmt2) {
  return (fmt1.codec === fmt2.codec);
};

var determineVideoFmt = function (sdp, formatPreference) {
  console.log('!!!3:', sdp);
    var lines = sdp.split('\n');
    var v_begin = lines.findIndex(function (line) {return line.startsWith('m=video');});
    if ((v_begin >= 0) && formatPreference.video) {
        var v_end = lines.slice(v_begin + 1).findIndex(function(line) {return line.startsWith('m=');});
            v_end = (v_end > 0 ? v_begin + v_end + 1 : lines.length);

        var v_lines = lines.slice(v_begin, v_end);

        var fmtcode = '';
        var fmts = [];
        var fmtcodes = v_lines[0].split(' ').slice(3);
        for (var i in fmtcodes) {
            var filter_lines = v_lines.filter(function (line) {
                return line.startsWith('a=rtpmap:'+fmtcodes[i]);
            });

            if (filter_lines.length === 0) continue;
            var fmtname = filter_lines[0].split(' ')[1].split('/');

            var fmt = {codec: fmtname[0].toLowerCase()};
            if (fmtname[0] === 'rtx') {
                var m_code = v_lines.filter(function(line) {return line.startsWith('a=fmtp:' + fmtcodes[i] + ' apt=');
                })[0].split(' ')[1].split('=')[1];
               fmts.push({code: fmtcodes[i], fmt: fmt, main_code: m_code});
            } else {
               fmts.push({code: fmtcodes[i], fmt: fmt});
            }

            if ((fmtcode === '') && formatPreference.video.preferred && isVideoFmtEqual(formatPreference.video.preferred, fmt)) {
                video_fmt = fmt;
                fmtcode = fmtcodes[i];
            }
        }

        if ((fmtcode === '') && formatPreference.video.optional) {
            for (var j in fmts) {
                if (formatPreference.video.optional.findIndex(function (o) {return isVideoFmtEqual(o, fmts[j].fmt);}) !== -1) {
                    video_fmt = fmts[j].fmt;
                    fmtcode = fmts[j].code;
                    break;
                }
            }
        }

        // if (video_fmt && video_fmt.codec === 'h264' && final_prf) {
        //     video_fmt.profile = final_prf;
        // }

        if (fmtcode) {
            var reserved_codes = [fmtcode];
            fmts.forEach(function (f) {
                if ((f.fmt.codec === 'red')
                    || (f.fmt.codec === 'ulpfec')
                    || ((f.fmt.codec === 'rtx') && (f.main_code === fmtcode))) {
                    reserved_codes.push(f.code);
                }
            });

            v_lines[0] = v_lines[0].split(' ').slice(0, 3).concat(reserved_codes).join(' ');
            v_lines = v_lines.filter(function(line) {
                return isReserve(line, reserved_codes);
            }).join('\n');
            return lines.slice(0, v_begin).concat(v_lines).concat(lines.slice(v_end)).join('\n');
        } else {
            log.info('No proper video format');
            return sdp;
        }
    }
    return sdp;
};

exports.getAudioSsrc = function (sdp) {
  var sdpObj = transform.parse(sdp)
  for (const media of sdpObj.media) {
    if (media.type == 'audio') {
      if (media.ssrcs && media.ssrcs[0]) {
        return media.ssrcs[0].id;
      }
    }
  }
};

exports.getVideoSsrcList = function (sdp) {
  var sdpObj = transform.parse(sdp);
  var ssrcList = [];
  for (const media of sdpObj.media) {
    if (media.type == 'video') {
      if (media.ssrcs) {
        media.ssrcs.forEach((ssrc) => {
          if (!ssrcList.includes(ssrc.id)) {
            ssrcList.push(ssrc.id);
          }
        });
      }
    }
  }
  return ssrcList;
};

exports.getSimulcastInfo = function (sdp) {
  var sdpObj = transform.parse(sdp);
  var simulcast = [];
  for (const media of sdpObj.media) {
    if (media.type == 'video') {
      if (media.simulcast) {
        const simInfo = media.simulcast;
        if (simInfo.dir1 === 'send') {
          simulcast = transform.parseSimulcastStreamList(simInfo.list1);
        }
      }
    }
  }
  return simulcast;
}

exports.processOffer = function (sdp, preference = {}, direction) {
  var sdpObj = transform.parse(sdp);
  var finalProfile = filterH264Payload(sdpObj, preference, direction);
  sdp = transform.write(sdpObj);
  audio_fmt = null;
  video_fmt = null;
  sdp = determineAudioFmt(sdp, preference);
  sdp = determineVideoFmt(sdp, preference);
  return { sdp, audioFormat:audio_fmt, videoFormat:video_fmt};
};

exports.processAnswer = function (sdp, preference = {}) {

};