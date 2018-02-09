'use strict';
var Fraction = require('fraction.js');

function Rational(num, den) {
  this.numerator = num;
  this.denominator = den;
}

const ZERO = new Rational(0, 1);
const ONE = new Rational(1, 1);
const ONE_THIRD = new Rational(1, 3);
const TWO_THIRDs = new Rational(2, 3);

function Rectangle({id = '', left, top, relativesize} = {}) {
  this.id = id;
  this.shape = 'rectangle',
  this.area = {
      left: left || ZERO,
      top: top || ZERO,
      width: relativesize || ZERO,
      height: relativesize || ZERO
  };
}

function generateFluidTemplates (maxInput) {
  var result = [];
  var maxDiv = Math.sqrt(maxInput);
  if (maxDiv > Math.floor(maxDiv))
    maxDiv = Math.floor(maxDiv) + 1;
  else
    maxDiv = Math.floor(maxDiv);

  for (var divFactor = 1; divFactor <= maxDiv; divFactor++) {
    var regions = [];
    var relativeSize = new Rational(1, divFactor);
    var id = 1;
    for (var y = 0; y < divFactor; y++)
      for(var x = 0; x < divFactor; x++) {
        var region = new Rectangle({
          id: String(id++),
          left: new Rational(x, divFactor),
          top: new Rational(y, divFactor),
          relativesize: relativeSize
        });
        regions.push(region);
      }

    result.push({region: regions});
  }
  return result;
}

function generateLectureTemplates (maxInput) {
  var result = [
    {region:[new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: ONE })]},
    {region:[
      new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: TWO_THIRDs}),
      new Rectangle({id: '2', left: TWO_THIRDs, top: ZERO, relativesize: ONE_THIRD}),
      new Rectangle({id: '3', left: TWO_THIRDs, top: ONE_THIRD, relativesize: ONE_THIRD}),
      new Rectangle({id: '4', left: TWO_THIRDs, top: TWO_THIRDs, relativesize: ONE_THIRD}),
      new Rectangle({id: '5', left: ONE_THIRD, top: TWO_THIRDs, relativesize: ONE_THIRD}),
      new Rectangle({id: '6', left: ZERO, top: TWO_THIRDs, relativesize: ONE_THIRD})
    ]}
  ];
  if (maxInput > 6) { // for maxInput: 8, 10, 12, 14
    var maxDiv = maxInput / 2;
    maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
    maxDiv = maxDiv > 7 ? 7 : maxDiv;

    for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
        var mainReginRelative = new Rational(divFactor - 1, divFactor);
        var minorRegionRelative = new Rational(1, divFactor);

        var regions = [new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative})];
        var id = 2;
        for (var y = 0; y < divFactor; y++) {
            regions.push(new Rectangle({
                id: ''+(id++),
                left: mainReginRelative,
                top: new Rational(y, divFactor),
                relativesize: minorRegionRelative,
            }));
        }

        for (var x = divFactor - 2; x >= 0; x--) {
            regions.push(new Rectangle({
                id: ''+(id++),
                left: new Rational(x, divFactor),
                top: mainReginRelative,
                relativesize: minorRegionRelative,
            }));
        }
        result.push({region: regions});
    }

    if (maxInput > 14) { // for maxInput: 17, 21, 25
      var maxDiv = (maxInput + 3) / 4;
      maxDiv = (maxDiv > Math.floor(maxDiv)) ? (maxDiv + 1) : maxDiv;
      maxDiv = maxDiv > 7 ? 7 : maxDiv;

      for (var divFactor = 4; divFactor <= maxDiv; divFactor++) {
        var mainReginRelative = new Rational(divFactor - 2, divFactor);
        var minorRegionRelative = new Rational(1, divFactor);

        var regions = [new Rectangle({id: '1', left: ZERO, top: ZERO, relativesize: mainReginRelative})];
        var id = 2;
        for (var y = 0; y < divFactor - 1; y++) {
          regions.push(new Rectangle({
            id: '' + (id++),
            left: mainReginRelative,
            top: new Rational(y, divFactor),
            relativesize: minorRegionRelative,
          }));
        }

        for (var x = divFactor - 3; x >= 0; x--) {
          regions.push(new Rectangle({
            id: '' + (id++),
            left: new Rational(x, divFactor),
            top: mainReginRelative,
            relativesize: minorRegionRelative,
          }));
        }

        for (var y = 0; y < divFactor; y++) {
          regions.push(new Rectangle({
            id: '' + (id++),
            left: new Rational(divFactor - 1, divFactor),
            top: new Rational(y, divFactor),
            relativesize: minorRegionRelative
          }));
        }

        for (var x = divFactor - 2; x >= 0; x--) {
          regions.push(new Rectangle({
            id: '' + (id++),
            left: new Rational(x, divFactor),
            top: new Rational(divFactor - 1, divFactor),
            relativesize: minorRegionRelative
          }));
        }
        result.push({region: regions});
      }
    }
  }

  return result;
}

function translateRational(str) {
  var frac = new Fraction(str);
  return new Rational(frac.n, frac.d);
}

exports.templateType = {
  FLUID: 'fluid',
  LECTURE: 'lecture',
  VOID: 'void'
};

exports.applyTemplate = function (type, maxInput, custom) {
  var layoutTemplates = [];
  switch (type) {
    case 'fluid':
      layoutTemplates = generateFluidTemplates(maxInput);
      break;
    case 'lecture':
      layoutTemplates = generateLectureTemplates(maxInput);
      break;
    case 'void':
      layoutTemplates = [];
      break;
    default:
      layoutTemplates = generateFluidTemplates(maxInput);
  }
  if (custom) {
    custom.map(function (tpl) {
      var len = tpl.region.length;
      tpl.region.forEach(function (reg) {
        if (reg.area) {
          reg.area.left = translateRational(reg.area.left);
          reg.area.top = translateRational(reg.area.top);
          reg.area.width = translateRational(reg.area.width);
          reg.area.height = translateRational(reg.area.height);
        }
      });
      var pos;
      for (var j in layoutTemplates) {
        if (layoutTemplates.hasOwnProperty(j)) {
          if (layoutTemplates[j].region.length >= len) {
            pos = j;
            break;
          }
        }
      }
      if (pos === undefined) {
        layoutTemplates.push(tpl);
      } else if (layoutTemplates[pos].region.length === len) {
        layoutTemplates.splice(pos, 1, tpl);
      } else {
        layoutTemplates.splice(pos, 0, tpl);
      }
    });
  }

  return layoutTemplates;
};
