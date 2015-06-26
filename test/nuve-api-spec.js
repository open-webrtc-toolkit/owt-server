
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
        debug("should create services with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createService("name","key", function (services) {
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
        debug("should create services with correct credentials");
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
        debug("should not create services with non-super credentials");
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
        debug("should get created service with correct credentials");
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
        debug("should get created service with correct credentials");
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
        debug("should get created service with correct credentials");
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
        debug("should get created service with correct credentials");
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
        debug("should get created service with correct credentials");
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

    it("Case MCU-1040 should get services with invalid credentials", function () {
        debug("should get created service with correct credentials");
        N.API.init("test", "test", 'http://localhost:3000/');
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


    it("Case MCU-1042 should not deleteService with error credentials", function () {
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
        debug("should deleteService with correct credentials");
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

    it("Case MCU-1037 get deleted service with correct credentials should triger error callback", function () {
        debug("get deleted service with correct credentials should triger error callback");
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

    it("should get services with correct credentials", function () {
        debug("should get created service with correct credentials");
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        var result;
        N.API.getServices(function (services) {
            result = JSON.parse(services);
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
            expect(result).not.toMatch(service);
        });
    });

        it("Case MCU-1043 get deleted unexisted service with correct credentials should triger error callback", function () {
        debug("get deleted service with correct credentials should triger error callback");
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

    it("Case MCU-1053 should list rooms", function () {
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

    it("Case MCU-756 should list rooms", function () {
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


    it("Case MCU-752 should create normal rooms", function () {
        N.API.createRoom(roomnormal.name, function(room) {
            id = room._id;
        });

        waitsFor(function () {
            return id !== undefined;
        }, "Nuve should have created the room", TIMEOUT);

        runs(function () {
            expect(id).toNotBe(undefined);
        });
    });

    it("Case MCU-767 should not create room with invalid mode", function () {
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

    it("Case MCU-1057 should not create room with invalid publishLimit with string character", function () {
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

    it("Case MCU-1066 should not create room with invalid publishLimit with big number", function () {
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
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

    it("Case MCU-1069 should not create room with invalid userLimit big number", function () {
        var callback = jasmine.createSpy("correctCallback");
        var errorCallback = jasmine.createSpy("errorCallback");
        N.API.createRoom('test1', function(room) {
            id = room._id;
            callback();
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

    it("Case MCU-1015 should not create room with invalid avCoordinated", function () {
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

    it("Case MCU-1060 should not create room with invalid maxInput", function () {
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


    it("should get normal rooms", function () {
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

    it("should create tokens for users in normal rooms", function () {
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user", "presenter", function(token) {
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

    it("should create tokens for users with special characters in normal rooms", function () {
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

    it("should create tokens for users with empty user name in normal rooms", function () {
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

    it("should create tokens for users with role as viewer in normal rooms", function () {
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user", "viewer", function(token) {
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

    it("should create tokens for users with error role in normal rooms", function () {
        var obtained = false;
        var received = false;
        N.API.createToken(id, "user", "role", function(token) {
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

        it("should create tokens error wrong room", function () {
        var obtained = false;
        var received = false;
        N.API.createToken('id', "user", "role", function(token) {
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

    it("should get users in normal rooms", function () {
        var obtained = false;
        var received = false;
        N.API.getUsers(id, function(token) {
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


    it("should delete normal rooms", function () {
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



     it("should error if delete a room already deleted", function () {
        var obtained = false;
        var received = false;
        N.API.deleteRoom(id, function(result) {
            obtained = false;
            received = true;
        }, function(error) {
            obtained = true;
            received = true;
        });
        waitsFor(function () {
            return received;
        }, "should delete normal rooms", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });

     it("should error if delete room with error id ", function () {
        var obtained = false;
        var received = false;
        N.API.deleteRoom("id", function(result) {
            obtained = false;
            received = true;
        }, function(error) {
            obtained = true;
            received = true;
        });
        waitsFor(function () {
            return received;
        }, "should delete normal rooms", TIMEOUT);

        runs(function () {
            expect(obtained).toBe(true);
        });
    });
    it("should create p2p rooms", function () {
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

    it("should get p2p rooms", function () {
        var obtained = false;
        var received = false;
        console.log("p2p room id is ", id);
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

    it("should delete p2p rooms", function () {
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
