
 var N = require('../dist/extras/basic_example/nuve');
 var https = require("https");
 var config = require('../dist/etc/woogeen_config');

 var TIMEOUT=10000,
     roomnormal = {name:'myTestRoom'},
     roomp2p = {name:'p2proom', p2p:true};

var id;
var service;
var sampleRooms;
var superServiceId = '_auto_generated_ID_';
var superServiceKey = '_auto_generated_KEY_';
var sampleServiceId = '_auto_generated_SAMPLE_ID_';
var sampleServiceKey = '_auto_generated_SAMPLE_KEY_';
var debug = function(msg) {
        console.log('++++ debug message:' + msg);
    };

describe("nuve", function () {

    beforeEach(function () {
        N.API.init(superServiceId, superServiceKey, 'http://localhost:3000/');
    });

    it("Case MCU-1035 should not create services with invalid credentials", function () {
         N.API.init("test",'1111', 'http://localhost:3000/');
        debug("MCU-1035 should create services with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createService("name","key", function (service) {
            callback();
            debug("createService", service);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });



    it("Case MCU-757 should create services with super credentials", function () {
        debug("MCU-757 should create services with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createService("name3","key", function (_service) {
            callback();
            service = _service;
            console.log("createService", service);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1033 should create service with an existed service name and key", function () {
        debug('MCU-1033 should create service with an existed service name and key');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createService("name3","key", function (_service) {
            callback();
            service = _service;
            console.log("createService", service);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1034 should not create services with non-super credentials", function () {
        debug("MCU-1034 should not create services with non-super credentials");
        N.API.init(sampleServiceId, sampleServiceKey, 'http://localhost:3000/');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createService("name3","key", function (_service) {
            callback();
            service = _service;
            console.log("createService", service);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });


    it("Case MCU-759 should get created service with super credentials", function () {
        debug("MCU-759 should get created service with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        console.log("should get created service with correct credentialse", service);
        N.API.getService(sampleServiceId, function (_service) {
            result = JSON.parse(_service);
            callback();
            console.log("should get created service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1036 should not get created service with non-super credentials", function () {
        debug("MCU-1036 should not get created service with non-super credentials");
        N.API.init(sampleServiceId, sampleServiceKey, 'http://localhost:3000/');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        console.log("should get created service with correct credentialse", service);
        N.API.getService(service, function (_service) {
            result = JSON.parse(_service);
            callback();
            console.log("should get created service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1038 should not get created service with invalid credentials", function () {
        debug("MCU-1038 should not get created service with invalid credentials");
        N.API.init("test", "test", 'http://localhost:3000/');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        console.log("should get created service with correct credentialse", service);
        N.API.getService(service, function (_service) {
            result = JSON.parse(_service);
            callback();
            console.log("should get created service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-758 should get services with correct credentials", function () {
        debug("MCU-758 should get created services with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
          console.log("should get services with correct credentials = service", service);
        N.API.getServices(function (services) {
            result = services;
            callback();
            console.log("should get services", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
            expect(result).toMatch(service);
        });
    });

    it("Case MCU-1039 should not get services with non-super credentials", function () {
        debug("MCU-1039 should not get created service with non-super credentials");
        N.API.init(sampleServiceId, sampleServiceKey, 'http://localhost:3000/');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
          console.log("should get services with correct credentials = service", service);
        N.API.getServices(function (services) {
            result = services;
            callback();
            console.log("should get services", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1040 should not get services with invalid credentials", function () {
        debug("MCU-1040 should not get services with invalid credentials");
        N.API.init("test", "test", 'http://localhost:3000/');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
          console.log("should not get services with correct credentials = service", service);
        N.API.getServices(function (services) {
            result = services;
            callback();
            console.log("should get services", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });


    it("Case MCU-1042 should not deleteService with error credentials", function () {
        debug('MCU-1042 should not deleteService with error credentials');
        N.API.init("test","1111", 'http://localhost:3000/');
        debug("should not deleteService with error credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.deleteService(service, function (_service) {
            var result = _service;
            callback();
            debug("should delete service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

     it("Case MCU-1041 should not deleteService with non-super credentials", function () {
        debug('MCU-1041 should not deleteService with non-super credentials')
        N.API.init(sampleServiceKey,sampleServiceId, 'http://localhost:3000/');
        debug("should not deleteService with error credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.deleteService(service, function (_service) {
            var result = _service;
            callback();
            debug("should delete service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1012 should deleteService with correct credentials", function () {
        debug("MCU-1012 should deleteService with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
          debug("should delete service", service);
        N.API.deleteService(service, function (_service) {
            var result = _service;
            callback();
            debug("should delete service", result);
        }, function(err){
            errorCallback();
        });
        waitsFor(function () {
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1037 should not get deleted service with correct credentials should triger error callback", function () {
        debug("MCU-1037 should not get deleted service with correct credentials should triger error callback");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        N.API.getService(service, function (_service) {
            result = _service;
            callback();
            debug("should get created service", result);
        }, function(err){
            debug("get error service", err);
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1043 not deleted unexisted service with correct credentials should triger error callback", function () {
        debug("MCU-1043 not deleted unexisted service with correct credentials should triger error callback");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        N.API.getService("service", function (_service) {
            result = _service;
            callback();
            debug("should get created service", result);
        }, function(err){
            debug("get error service", err);
            errorCallback();
        });
        waitsFor(function () {
           return errorCallback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });


    it("Case MCU-1052 should not list rooms with wrong credentials", function () {
        debug('MCU-1052 should not list rooms with wrong credentials');
        var rooms;
        var received = false, ok = false;

        N.API.init("test", "1111", 'http://localhost:3000/');

        N.API.getRooms(function(rooms_) {
            received = true;
            ok = true;
            console.log("true");
        }, function(error) {
            received = true;
            ok = false;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have answered", TIMEOUT);

        runs(function () {
            expect(ok).toBe(false);
        });
    });

    it("Case MCU-1053 should list rooms with super credential", function () {
        debug('MCU-1053 should list rooms with super credential');
        var rooms;
        var received = false, ok = false;

        N.API.getRooms(function(rooms_) {
            received = true;
            ok = true;
        }, function(error) {
            received = true;
            ok = false;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have answered", TIMEOUT);

        runs(function () {
            expect(ok).toBe(true);
        });
    });

    it("Case MCU-756 should list rooms with non-super credential", function () {
        debug("MCU-756 should list rooms with non-super credential");
        var rooms;
        N.API.init(sampleServiceId, sampleServiceKey, 'http://localhost:3000/');
        var received = false, ok = false;

        N.API.getRooms(function(rooms_) {
            sampleRooms = rooms_;
            received = true;
            ok = true;
        }, function(error) {
            received = true;
            ok = false;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have answered", TIMEOUT);

        runs(function () {
            expect(ok).toBe(true);
        });
    });

    it("Case MCU-752 should create normal room with super credential", function () {
        debug('MCU-752 should create normal room with super credential');
        N.API.createRoom(roomnormal.name, function(room) {
            id = room._id;
            console.log('room id',id);
        });

        waitsFor(function () {
            return id !== undefined;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(id).toNotBe(undefined);
        });
    });

    it("Case MCU-1016 should create normal room with the same name", function () {
        debug('MCU-1016 should create normal room with the same name');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            N.API.createRoom(roomnormal.name,function(room){
                callback();
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return callback.callCount>0;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(callback).toHaveBeenCalled();
                expect(errorCallback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1169 should create normal room with correct mediaMixing(set resolutio to 'hd720p')", function () {
        debug('MCU-1016 should create normal room with the same name');
            var obtained=false;
            var received=false;
            N.API.createRoom('test17',function(room){
               obtained=true;
               received=true;
            },function(err){
                received=true;
                obtained=false;
            },{
               'mediaMixing':{
                'video':{
                    'resolutio':'hd720p'
                }
              }
            });
            waitsFor(function(){
                return received;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(obtained).toBe(true);
            });
    });

    it("Case MCU-1170 should create normal room with correct mediaMixing(set resolutio to 'hd1080p')", function () {
        debug('MCU-1016 should create normal room with the same name');
            var obtained=false;
            var received=false;
            N.API.createRoom('test18',function(room){
               obtained=true;
               received=true;
            },function(err){
                received=true;
                obtained=false;
            },{
               'mediaMixing':{
                'video':{
                    'resolutio':'hd1080p'
                }
              }
            });
            waitsFor(function(){
                return received;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(obtained).toBe(true);
            });
    });

    it("Case MCU-1171 should create normal room with correct mediaMixing(set bitrate to 300)", function () {
        debug('MCU-1016 should create normal room with the same name');
            var obtained=false;
            var received=false;
            N.API.createRoom('test18',function(room){
               obtained=true;
               received=true;
            },function(err){
                received=true;
                obtained=false;
            },{
               'mediaMixing':{
                'video':{
                    'bitrate':300
                }
              }
            });
            waitsFor(function(){
                return received;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(obtained).toBe(true);
            });
    });

    it("Case MCU-1172 should create normal room with correct mediaMixing(set bkColor to otherColor)", function () {
        debug('MCU-1016 should create normal room with the same name');
            var obtained=false;
            var received=false;
            N.API.createRoom('test18',function(room){
               obtained=true;
               received=true;
            },function(err){
                received=true;
                obtained=false;
            },{
               'mediaMixing':{
                'video':{
                    'bkColor':{
                        "r":255,
                        "g":66,
                        "b":66
                    }
                }
              }
            });
            waitsFor(function(){
                return received;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(obtained).toBe(true);
            });
    });

    it("Case MCU-1173 should create normal room with correct mediaMixing(set bkColor to otherColor)", function () {
        debug('MCU-1016 should create normal room with the same name');
            var obtained=false;
            var received=false;
            N.API.createRoom('test18',function(room){
               obtained=true;
               received=true;
            },function(err){
                received=true;
                obtained=false;
            },{
               'mediaMixing':{
                'video':{
                    'bkColor':{
                        "r":71,
                        "g":71,
                        "b":71
                    }
                }
              }
            });
            waitsFor(function(){
                return received;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(obtained).toBe(true);
            });
    });

    it("Case MCU-1154 should not create normal room with invalid name", function () {
        debug('MCU-1154 should not create normal room with invalid name');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom(1234,function(room){
                callback();
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1155 should not create normal room with invalid name", function () {
        debug('MCU-1155 should not create normal room with invalid name');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom({},function(room){
                callback();
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-753 should update room info with correct parameters", function () {
        debug('MCU-753 should update room info with correct parameters');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
                publishLimit:15
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result,"room id is",id);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return callback.callCount>0;
            },"should update room info with correct parameters",TIMEOUT);

            runs(function(){
                expect(callback).toHaveBeenCalled();
                expect(errorCallback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1023 should not update room info with invalid roomId", function () {
        debug('MCU-1023 should not update room info with invalid roomId');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom("test3",{
                userLimit:20
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with invalid roomId",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();
            });
    });

    it("Case MCU-1024 should not update room info with invalid room mode", function () {
        debug('MCU-1024 should not update room info with invalid room mode');
            var callback=jasmine.createSpy("callback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
                mode:"test4"
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with invalid room mode",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1032 should not update room info with room mediaMixing param (invalid layout)", function () {
        debug('MCU-1032 should not update room info with room mediaMixing param (invalid layout)');
            var callback=jasmine.createSpy("callback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    layout:"test5"
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid layout)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1031 should not update room info with room mediaMixing param (invalid bkColor)", function () {
        debug('MCU-1031 should not update room info with room mediaMixing param (invalid bkColor)');
            var callback=jasmine.createSpy("callback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    bkColor:"c"
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid bkColor)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1030 should not update room info with room mediaMixing param (invalid bitrate)", function () {
        debug('MCU-1030 should not update room info with room mediaMixing param (invalid bitrate)');
            var callback=jasmine.createSpy("callback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    bitrate:"test6"
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid bitrate)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1029 should not update room info with room mediaMixing param (invalid resolution)", function () {
        debug('MCU-1029 should not update room info with room mediaMixing param (invalid resolution)');
            var callback=jasmine.createSpy("callback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    resolution:"test7"
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid resolution)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1028 should not update room info with room mediaMixing param (invalid maxInput)", function () {
        debug('MCU-1028 should not update room info with room mediaMixing param (invalid maxInput)');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    maxInput:"test8"
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid maxInput)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1027 should not update room info with room mediaMixing param (invalid avCoordinated)", function () {
        debug('MCU-1027 should not update room info with room mediaMixing param (invalid avCoordinated');
            var callback=jasmine.createSpy("coreectcallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.updateRoom(id,{
              mediaMixing:{
                video:{
                    avCoordinated:3
                }
              }
            },function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            });
            waitsFor(function(){
                return callback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid avCoordinated)",TIMEOUT);

            runs(function(){
                expect(callback).toHaveBeenCalled();
                expect(errorCallback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-767 should not create room with invalid mode", function () {
        debug('MCU-767 should not create room with invalid mode');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mode" : "test"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

     it("Case MCU-1059 should not create room with invalid mode", function () {
        debug('MCU-1059 should not create room with invalid mode');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mode" : 1234
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1158 should create room with specified publishLimit", function () {
        debug('MCU-1158 should create room with specified publishLimit');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        N.API.createRoom('test1', function(room) {
            callback();
            console.log('createRoom with specified publishLimit',room);
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : 30
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1162 should create room with specified userLimit", function () {
        debug("MCU-1162 should create room with specified userLimit");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom("test15", function(room) {
            callback();
            console.log('create room with specified userLimit',room)
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : 30
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });
    it("Case MCU-1057 should not create room with invalid publishLimit with string character", function () {
        debug('MCU-1057 should not create room with invalid publishLimit with string character');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : "test"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1066 should create room with invalid publishLimit with big number", function () {
        debug('MCU-1066 should create room with invalid publishLimit with big number');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            callback();
            console.log('create room with big number of publishLimit',room);
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : 1000000000
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).not.toHaveBeenCalled();
            expect(callback).toHaveBeenCalled();
        });
    });

    it("Case MCU-1063 should not create room with invalid publishLimit string number", function () {
        debug('MCU-1063 should not create room with invalid publishLimit string number');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : "-100"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1064 should not create room with invalid publishLimit with object", function () {
        debug('MCU-1064 should not create room with invalid publishLimit with object');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : {}
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1065 should not create room with invalid publishLimit with number less than -1", function () {
        debug('MCU-1065 should not create room with invalid publishLimit with number less than -1');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "publishLimit" : -4
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1058 should not create room with invalid userLimit with string character", function () {
        debug('MCU-1058 should not create room with invalid userLimit with string character');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : "test"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1067 should not create room with invalid userLimit string number", function () {
        debug('MCU-1067 should not create room with invalid userLimit string number');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : "-100"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1068 should not create room with invalid userLimit object", function () {
        debug('MCU-1068 should not create room with invalid userLimit object');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : {}
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1069 should create room with invalid userLimit big number", function () {
        debug('MCU-1069 should create room with invalid userLimit big number');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
            console.log('create room with userLimit big number',room);
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : 10000000
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).not.toHaveBeenCalled();
            expect(callback).toHaveBeenCalled();
        });
    });

    it("Case MCU-1070 should not create room with invalid userLimit with number less than -1", function () {
        debug('MCU-1070 should not create room with invalid userLimit with number less than -1');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "userLimit" : -4
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1071 should not create room with invalid mediaMixing with string", function () {
        debug('MCU-1071 should not create room with invalid mediaMixing with string');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing" : "test"
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1072 should not create room with invalid mediaMixing with number", function () {
        debug('MCU-1072 should not create room with invalid mediaMixing with number');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing" : 123
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1073 should not create room with invalid video", function () {
        debug('MCU-1073 should not create room with invalid video');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : 4
            }
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });
//////////////////////////////////
    it("Case MCU-1015 should create room with invalid avCoordinated", function () {
        debug('MCU-1015 should create room with invalid avCoordinated');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "avCoordinated" : "5"
                }
            }
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).not.toHaveBeenCalled();
            expect(callback).toHaveBeenCalled();
        });
    });

    xit("Case MCU-1015 should create room with different avCoordinated", function () {
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "avCoordinated" : "1"
                }
            }
        });

        waitsFor(function () {
            return callback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).not.toHaveBeenCalled();
            expect(callback).toHaveBeenCalled();
        });
    });

    it("Case MCU-1017 should not create room with invalid maxInput", function () {
        debug('MCU-1017 should not create room with invalid maxInput');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "maxInput" : "test"
                }
            }
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1018 should not create room  with room mediaMixing param (invalid resolution)", function () {
        debug('MCU-1018 should not create room  with room mediaMixing param (invalid resolution)');
            var callback=jasmine.createSpy("correctCallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom("testname1",function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            },{
              "mediaMixing":{
                "video":{
                    "resolution":"test8"
                }
              }
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not cteate room with room mediaMixing param (invalid resolution)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1019 should not create room with room mediaMixing param (invalid bitrate)", function () {
        debug('MCU-1019 should not create room with room mediaMixing param (invalid bitrate)');
            var callback=jasmine.createSpy("correctCallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom("testname2",function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            },{
              'mediaMixing':{
                'video':{
                    'bitrate':"test9"
                }
              }
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not create room with room mediaMixing param (invalid bitrate)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1020 should not create room with room mediaMixing param (invalid bkColor)", function () {
        debug('MCU-1020 should not create room with room mediaMixing param (invalid bkColor)');
            var callback=jasmine.createSpy("correctCallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom("testname3",function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            },{
              'mediaMixing':{
                'video':{
                    'bkColor':"c"
                }
              }
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not cteate room with room mediaMixing param (invalid bkColor)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1021 should not create room with room mediaMixing param (invalid layout)", function () {
        debug('MCU-1021 should not create room with room mediaMixing param (invalid layout)');
            var callback=jasmine.createSpy("correctCallback");
            var errorCallback=jasmine.createSpy("errorCallback");
            var result;
            N.API.createRoom("testname4",function(room){
                result = JSON.parse(room);
                callback();
                console.log("should update", result);
            },function(){
                errorCallback();
            },{
              'mediaMixing':{
                'video':{
                    'layout':"test9"
                }
              }
            });
            waitsFor(function(){
                return errorCallback.callCount>0;
            },"should not update room info with room mediaMixing param (invalid layout)",TIMEOUT);

            runs(function(){
                expect(errorCallback).toHaveBeenCalled();
                expect(callback).not.toHaveBeenCalled();

            });
    });

    it("Case MCU-1060 should not create room with invalid maxInput", function () {
        debug('MCU-1060 should not create room with invalid maxInput');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "maxInput" : "-1"
                }
            }
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1061 should not create room with invalid maxInput", function () {
        debug(' MCU-1061 should not create room with invalid maxInput');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "maxInput" : -1
                }
            }
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1062 should not create room with invalid maxInput", function () {
        debug('MCU-1062 should not create room with invalid maxInput');
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            console.log("Case 1015 create room success:", room);
            callback();
        }, function(err) {
            errorCallback();
        }, {
            "mediaMixing": {
                "video" : {
                    "maxInput" : {}
                }
            }
        });

        waitsFor(function () {
            return errorCallback.callCount > 0;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-754 should get room with correct credential", function () {
        debug('MCU-754 should get room with correct credential');
        var obtained = false;
        var received = false;
        N.API.getRoom(id, function(result) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1047 should not get room with invalid credential", function () {
        debug('MCU-1047 should not get room with invalid credential');
        N.API.init("test10","2222","http://localhost:3001")
        var obtained = false;
        var received = false;
        N.API.getRoom(id, function(result) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(false);
        });
    });

    it("Case MCU-1048 should not get room with invalid roomId", function () {
        debug('MCU-1048 should not get room with invalid roomId');
        var obtained = false;
        var received = false;
        N.API.getRoom("test11", function(result) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(false);
        });
    });

    it("Case MCU-1050 should get room with different and coreect credential", function () {
        debug('MCU-1050 should get room with different and coreect credential');
        N.API.init(sampleServiceId,sampleServiceKey,"http://localhost:3001/")
        var obtained = false;
        var received = false;
        N.API.getRoom(id, function(room) {
            obtained = true;
            received = true;
            var result = JSON.parse(room);
            console.log("different credential room info",result)
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(false);

        });
    });

    it("Case MCU-1014 should create token for user in normal room", function () {
        debug('MCU-1014 should create token for user in normal room');
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user", "presenter", function(token) {
            console.log('create token in room',id);
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1174 should create token for user with special characters in normal rooms", function () {
        debug('MCU-1174 should create token for user with special characters in normal rooms');
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user_with_$?üóñ", "presenter", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1045 should not create token with invalid participant's name", function () {
        debug("MCU-1045 should not create token with invalid participant's name");
        var obtained = false;
        var received = false;
        N.API.createToken(id, 1234, "presenter", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1179 should create tokens for users with empty user name in normal rooms", function () {
        debug('MCU-1179 should create tokens for users with empty user name in normal rooms');
        var obtained = false;
        var received = false;
        N.API.createToken(id, "", "presenter", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1180 should create tokens for users with role as viewer in normal rooms", function () {
        debug('MCU-1180 should create tokens for users with role as viewer in normal rooms');
        var obtained = false;
        var received = false;

        N.API.createToken(id, "user1", "viewer", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1046 should create token with invalid role", function () {
        debug('MCU-1046 should create token with invalid role');
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user2", "role", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1044 should not create token with invalid roomId", function () {
        debug('MCU-1044 should not create token with invalid roomId');
        var obtained = false;
        var received = false;
        N.API.createToken('id', "user", "presenter", function(token) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(false);
        });
    });

    it("Case MCU-1054 deleteRoom with invalid credential", function () {
        debug('MCU-1054 deleteRoom with invalid credential');
        N.API.init("testid","testkey","http://localhost:3001/");
        var callback = jasmine.createSpy("callback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.deleteRoom(id, function(result) {
            callback();
        }, function(err) {
            errorCallback();
        });

        waitsFor(function () {
            return errorCallback.callCount>0;
        }, "deleteRoom unexistent room should be get error message", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1055 should not deleteRoom with invalid roomId(not existed)", function () {
        debug('MCU-1055 should not deleteRoom with invalid roomId(not existed)');
        var callback = jasmine.createSpy("callback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.deleteRoom('test16', function(result) {
            callback();
        }, function(err) {
            errorCallback();
        });

        waitsFor(function () {
            return errorCallback.callCount>0;
        }, "deleteRoom unexistent room should be get error message", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });

    it("Case MCU-1011 should delete normal room(test deleteRoom)", function () {
        debug('MCU-1011 should delete normal room(test deleteRoom');
        var obtained = false;
        var received = false;
        N.API.deleteRoom(id, function(result) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "should delete normal rooms", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

    it("Case MCU-1153 should create room with 'p2p'", function () {
        debug("MCU-1153 should create room with 'p2p'");
        var obtained = false;
        var received = false;
        N.API.createRoom(roomp2p.name, function(room) {
            id = room._id;
            debug("create p2p room ", id);
            obtained = true;
            received = true;
        }, function() {
            obtained = false;
            received = true;

        });

        waitsFor(function () {
           return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(id).toNotBe(undefined);
        });
    });

    it("Case MCU-1056 should not deleteRoom with different and correct credential", function () {
        debug('MCU-1056 should not deleteRoom with different and correct credential');
        N.API.init(sampleServiceId,sampleServiceKey,"http://localhost:3001/");
        var callback = jasmine.createSpy("callback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.deleteRoom(id, function(result) {
            callback();
        }, function(err) {
            errorCallback();
        });

        waitsFor(function () {
            return errorCallback.callCount>0;
        }, "deleteRoom unexistent room should be get error message", TIMEOUT);

        runs(function () {
            expect(errorCallback).toHaveBeenCalled();
            expect(callback).not.toHaveBeenCalled();
        });
    });


    it("Case MCU-1181 should get p2p rooms", function () {
        debug('MCU-1181 should get p2p rooms');
        var obtained = false;
        var received = false;
        N.API.getRoom(id, function(result) {
            obtained = true;
            received = true;
        }, function(error) {
            obtained = false;
            received = true;
        });

        waitsFor(function () {
            return received;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

   xit("Case MCU-1181 should get p2p rooms", function () {
        debug('MCU-1181 should get p2p rooms');
        var obtained;
        N.API.getRoom(id, function(room) {
            obtained = true;
            var result = JSON.parse(room);
            console.log('p2proom is',result);
        }, function(error) {
            obtained = false;
        });

        waitsFor(function () {
            return true;
        }, "", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(false);
        });
    });
    it("Case MCU-1182 should delete p2p rooms", function () {
        debug('MCU-1182 should delete p2p rooms');
        N.API.deleteRoom(id, function(result) {
            id = undefined;
        }, function(error) {

        });

        waitsFor(function () {
            return id === undefined;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(id).toBe(undefined);
        });
    });
});