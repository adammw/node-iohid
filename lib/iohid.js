var iohid = require('../build/Release/iohid');
var events = require('events');

inherits(iohid.Manager, events.EventEmitter);
exports.Manager = iohid.Manager;

// extend prototype
function inherits(target, source) {
  for (var k in source.prototype)
    target.prototype[k] = source.prototype[k];
}