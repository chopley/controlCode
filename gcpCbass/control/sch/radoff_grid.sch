do PointingOffset azoff= 0, 10, 1
{
  offset az=$azoff
  until $acquired(source)
}
