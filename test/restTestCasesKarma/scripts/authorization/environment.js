icsREST = require('./exportsHeader');
icsREST.API.init('5cb3e6e69ea30e73d1235c73', 'BvhCHLseqrJnyWpRvVTp6TqE2GxMOCjeZD3LvnCt/WA1LegUiH33g+VEdZgpSevLfwa33krnoKIEW9bAaTLZULQDRk6RFPqOWOIhzUS30/EZkDQFuCMU9ztRadk6s7fXLb3cMuShV7JAKOwAuQqXND+6c1RzTpREVFcY0u7Jioo=', 'http://localhost:3000/', true);
var authorization=icsREST.API.sendHeader();
console.log("authorization : ",authorization);
