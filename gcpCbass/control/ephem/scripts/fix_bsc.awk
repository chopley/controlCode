{
  if ($8 < 0) {
    pmra_sign = "-";
    pmra = -$8 / 15.0;
  } else {
    pmra_sign = "+";
    pmra = $8 / 15.0;
  }
  if ($9 < 0) {
    pmdec_sign = "-";
    pmdec = -$9;
  } else {
    pmdec_sign = "+";
    pmdec = $9;
  }
  printf("J2000 %-9s %s:%s:%s %s:%s:%s %s0:0:%07.4f %s0:0:%06.3f # %3g\n", $1,$2,$3,$4,$5,$6,$7,pmra_sign, pmra, pmdec_sign, pmdec,$10)
}
