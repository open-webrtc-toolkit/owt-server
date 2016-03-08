var config = {};

config.rabbit = {};
config.rabbit.host = 'localhost';
config.rabbit.port = 5672;

config.initial_time = 10 * 1000/*MS*/;
config.check_alive_period = 1000/*MS*/;
config.check_alive_count = 10;
config.schedule_reserve_time = 60 * 1000/*MS*/;

config.strategy = {};
config.strategy.general = 'last-used';
config.strategy.portal = 'last-used';
config.strategy.webrtc = 'last-used';
config.strategy.rtsp = 'round-robin';
config.strategy.file = 'randomly-pick';
config.strategy.audio = 'most-used';
config.strategy.video = 'least-used';

module.exports = config;
