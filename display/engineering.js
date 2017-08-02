// Print a number using engineering units: micro, milli, nano, Kilo, etc.
function numberToEngineeringString(number, showLong) {
  if (isNaN(number)) {
    return NaN;
  } else if (number == 0) {
    return '0.0';
  }
  let prefix = '';
  if (number < 0) {
    number = -number;
    prefix = '-';
  }
  const p3 = Math.floor(Math.log10(number)/3);
  if (p3 < -5) {
    return prefix + 'Tiny';
  } else if (p3 > 5) {
    return prefix + 'Huge';
  }
  const n3 = number * 10**(-p3 * 3);
  return prefix + n3.toPrecision(4) + ' ' +
    (showLong ? longEngineeringSuffix(p3) : shortEngineeringSuffix(p3));
}

function shortEngineeringSuffix(power3) {
  const suffixes = ['a', 'f', 'p', 'n', '\u03BC', 'm', '',
                    'k', 'M', 'G', 'T', 'P', 'E']
  return engineeringSuffix(power3, suffixes);
}

function longEngineeringSuffix(power3) {
  const suffixes = ['atto',
                    'femto',
                    'pico',
                    'nano',
                    'micro',
                    'milli',
                    '',
                    'kilo',
                    'mega',
                    'giga',
                    'tera',
                    'peta',
                    'exa'];
  return engineeringSuffix(power3, suffixes);
}

function engineeringSuffix(power3, suffixes) {
  if ((power3 < -6) || (power3 > 6)) {
    return '?';
  } else {
    return suffixes[power3 + 6];
  }
}
