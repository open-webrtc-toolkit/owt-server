
 var N = require('../dist/extras/basic_example/nuve');
 var https = require("https");
 var config = require('../dist/etc/woogeen_config');

 var TIMEOUT=10000,
     roomnormal = {name:'myTestRoom'},
     roomp2p = {name:'p2proom', p2p:true};

var id;
var service;
var debug = function(msg) {
        console.log('++++ debug message:' + msg);
    };

describe("nuve", function () {

    beforeEach(function () {
        N.API.init('_auto_generated_ID_', '_auto_generated_KEY_', 'http://localhost:3000/');
    });

    it("should not create services with error credentials", function () {
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



    it("should create services with correct credentials", function () {
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


    it("should get created service with correct credentials", function () {
        debug("should get created service with correct credentials");
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
           return callback.callCount > 0;
        }, "should create services with correct credentials", TIMEOUT);

        runs(function () {
            expect(callback).toHaveBeenCalled();
            expect(errorCallback).not.toHaveBeenCalled();
        });
    });

    it("should get services with correct credentials", function () {
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
    it("should not deleteService with error credentials", function () {
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


    it("should deleteService with correct credentials", function () {
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

    it("get deleted service with correct credentials should triger error callback", function () {
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

        it("get deleted unexisted service with correct credentials should triger error callback", function () {
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


    it("should not list rooms with wrong credentials", function () {
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

    it("should list rooms", function () {
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

    it("should create normal rooms", function () {
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
