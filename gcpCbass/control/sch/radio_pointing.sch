(Time lststart, Time lststop)
# Cycle through list of radio sources taking a cross-scan on each

# sources must be really bright

import ~cbass/gcpCbass/control/sch/schedlib.sch

# Define source list, with an lst for each one.  This list is
# created by rad_set.m in order to maximize sky coverage.

group Source {
  Source name,    # source name
  Time  lst,      # lst to observe source
  Double flux     # source flux (in Jy)
}

listof Source sources = {
{J0319+415, 0.00, 23.30},
{    CasA, 0.05, 670.00},
{J0319+415, 0.10, 23.30},
{    CasA, 0.15, 670.00},
{J0319+415, 0.20, 23.30},
{    CasA, 0.25, 670.00},
{    TauA, 0.30, 615.00},
{    CasA, 0.35, 670.00},
{    CasA, 0.40, 670.00},
{    CasA, 0.45, 670.00},
{    CasA, 0.50, 670.00},
{    CasA, 0.55, 670.00},
{    CygA, 0.60, 365.00},
{J0319+415, 0.65, 23.30},
{J0319+415, 0.70, 23.30},
{    CygA, 0.75, 365.00},
{    CygA, 0.80, 365.00},
{    TauA, 0.85, 615.00},
{    TauA, 0.90, 615.00},
{    CygA, 0.95, 365.00},
{J0319+415, 1.00, 23.30},
{    CygA, 1.05, 365.00},
{J0319+415, 1.10, 23.30},
{    CygA, 1.15, 365.00},
{    CasA, 1.20, 670.00},
{    CygA, 1.25, 365.00},
{    CasA, 1.30, 670.00},
{J0319+415, 1.35, 23.30},
{    TauA, 1.40, 615.00},
{    CygA, 1.45, 365.00},
{    CasA, 1.50, 670.00},
{    CasA, 1.55, 670.00},
{    TauA, 1.60, 615.00},
{J0319+415, 1.65, 23.30},
{    CygA, 1.70, 365.00},
{    CasA, 1.75, 670.00},
{J0319+415, 1.80, 23.30},
{    CygA, 1.85, 365.00},
{    TauA, 1.90, 615.00},
{    CasA, 1.95, 670.00},
{    CygA, 2.00, 365.00},
{    CygA, 2.05, 365.00},
{    CygA, 2.10, 365.00},
{    CygA, 2.15, 365.00},
{    TauA, 2.20, 615.00},
{    TauA, 2.25, 615.00},
{    CygA, 2.30, 365.00},
{J0319+415, 2.35, 23.30},
{    TauA, 2.40, 615.00},
{    TauA, 2.45, 615.00},
{    CasA, 2.50, 670.00},
{    CasA, 2.55, 670.00},
{    TauA, 2.60, 615.00},
{    TauA, 2.65, 615.00},
{    TauA, 2.70, 615.00},
{    TauA, 2.75, 615.00},
{    CasA, 2.80, 670.00},
{    CasA, 2.85, 670.00},
{    TauA, 2.90, 615.00},
{    CasA, 2.95, 670.00},
{    TauA, 3.00, 615.00},
{J0319+415, 3.05, 23.30},
{    TauA, 3.10, 615.00},
{J0319+415, 3.15, 23.30},
{    CasA, 3.20, 670.00},
{J0319+415, 3.25, 23.30},
{    CasA, 3.30, 670.00},
{    TauA, 3.35, 615.00},
{J0319+415, 3.40, 23.30},
{    CasA, 3.45, 670.00},
{    CasA, 3.50, 670.00},
{    CasA, 3.55, 670.00},
{    CasA, 3.60, 670.00},
{    CasA, 3.65, 670.00},
{J0319+415, 3.70, 23.30},
{    TauA, 3.75, 615.00},
{    CasA, 3.80, 670.00},
{J0319+415, 3.85, 23.30},
{J0319+415, 3.90, 23.30},
{J0319+415, 3.95, 23.30},
{J0319+415, 4.00, 23.30},
{    CasA, 4.05, 670.00},
{    CasA, 4.10, 670.00},
{    CasA, 4.15, 670.00},
{J0319+415, 4.20, 23.30},
{    TauA, 4.25, 615.00},
{J0319+415, 4.30, 23.30},
{    TauA, 4.35, 615.00},
{    CasA, 4.40, 670.00},
{    TauA, 4.45, 615.00},
{    CasA, 4.50, 670.00},
{    TauA, 4.55, 615.00},
{J0319+415, 4.60, 23.30},
{    CasA, 4.65, 670.00},
{    TauA, 4.70, 615.00},
{    CasA, 4.75, 670.00},
{J0319+415, 4.80, 23.30},
{    TauA, 4.85, 615.00},
{    CasA, 4.90, 670.00},
{J0319+415, 4.95, 23.30},
{    CasA, 5.00, 670.00},
{    CasA, 5.05, 670.00},
{J0319+415, 5.10, 23.30},
{    TauA, 5.15, 615.00},
{J0319+415, 5.20, 23.30},
{    TauA, 5.25, 615.00},
{    CasA, 5.30, 670.00},
{J0319+415, 5.35, 23.30},
{    TauA, 5.40, 615.00},
{    TauA, 5.45, 615.00},
{    TauA, 5.50, 615.00},
{    TauA, 5.55, 615.00},
{    CasA, 5.60, 670.00},
{    CasA, 5.65, 670.00},
{    CasA, 5.70, 670.00},
{    TauA, 5.75, 615.00},
{    CasA, 5.80, 670.00},
{J0319+415, 5.85, 23.30},
{J0319+415, 5.90, 23.30},
{J0319+415, 5.95, 23.30},
{    TauA, 6.00, 615.00},
{J0319+415, 6.05, 23.30},
{J0319+415, 6.10, 23.30},
{J0319+415, 6.15, 23.30},
{    TauA, 6.20, 615.00},
{    TauA, 6.25, 615.00},
{    TauA, 6.30, 615.00},
{    TauA, 6.35, 615.00},
{    CasA, 6.40, 670.00},
{J0319+415, 6.45, 23.30},
{    CasA, 6.50, 670.00},
{J0319+415, 6.55, 23.30},
{    CasA, 6.60, 670.00},
{    TauA, 6.65, 615.00},
{    TauA, 6.70, 615.00},
{    TauA, 6.75, 615.00},
{    CasA, 6.80, 670.00},
{    CasA, 6.85, 670.00},
{    CasA, 6.90, 670.00},
{    TauA, 6.95, 615.00},
{    TauA, 7.00, 615.00},
{J0319+415, 7.05, 23.30},
{J0319+415, 7.10, 23.30},
{J0319+415, 7.15, 23.30},
{    TauA, 7.20, 615.00},
{    TauA, 7.25, 615.00},
{    TauA, 7.30, 615.00},
{    TauA, 7.35, 615.00},
{J0319+415, 7.40, 23.30},
{J0319+415, 7.45, 23.30},
{J0319+415, 7.50, 23.30},
{J0319+415, 7.55, 23.30},
{    TauA, 7.60, 615.00},
{    TauA, 7.65, 615.00},
{    TauA, 7.70, 615.00},
{  VirgoA, 7.75, 70.00},
{  VirgoA, 7.80, 70.00},
{    TauA, 7.85, 615.00},
{    TauA, 7.90, 615.00},
{J0319+415, 7.95, 23.30},
{    TauA, 8.00, 615.00},
{J0319+415, 8.05, 23.30},
{  VirgoA, 8.10, 70.00},
{J0319+415, 8.15, 23.30},
{J0319+415, 8.20, 23.30},
{  VirgoA, 8.25, 70.00},
{J0319+415, 8.30, 23.30},
{    TauA, 8.35, 615.00},
{    TauA, 8.40, 615.00},
{  VirgoA, 8.45, 70.00},
{J0319+415, 8.50, 23.30},
{J0319+415, 8.55, 23.30},
{J1229+020, 8.60, 30.00},
{  VirgoA, 8.65, 70.00},
{    TauA, 8.70, 615.00},
{    TauA, 8.75, 615.00},
{J1229+020, 8.80, 30.00},
{    TauA, 8.85, 615.00},
{J0319+415, 8.90, 23.30},
{    TauA, 8.95, 615.00},
{  VirgoA, 9.00, 70.00},
{    TauA, 9.05, 615.00},
{J1229+020, 9.10, 30.00},
{J1229+020, 9.15, 30.00},
{J1229+020, 9.20, 30.00},
{  VirgoA, 9.25, 70.00},
{  VirgoA, 9.30, 70.00},
{J1229+020, 9.35, 30.00},
{J0319+415, 9.40, 23.30},
{    TauA, 9.45, 615.00},
{J0319+415, 9.50, 23.30},
{J0319+415, 9.55, 23.30},
{J1229+020, 9.60, 30.00},
{    TauA, 9.65, 615.00},
{J1229+020, 9.70, 30.00},
{    TauA, 9.75, 615.00},
{  VirgoA, 9.80, 70.00},
{  VirgoA, 9.85, 70.00},
{  VirgoA, 9.90, 70.00},
{  VirgoA, 9.95, 70.00},
{J1229+020, 10.00, 30.00},
{    TauA, 10.05, 615.00},
{    TauA, 10.10, 615.00},
{  VirgoA, 10.15, 70.00},
{J1229+020, 10.20, 30.00},
{J1229+020, 10.25, 30.00},
{  VirgoA, 10.30, 70.00},
{    TauA, 10.35, 615.00},
{J1229+020, 10.40, 30.00},
{  VirgoA, 10.45, 70.00},
{  VirgoA, 10.50, 70.00},
{    TauA, 10.55, 615.00},
{J1229+020, 10.60, 30.00},
{J1229+020, 10.65, 30.00},
{  VirgoA, 10.70, 70.00},
{  VirgoA, 10.75, 70.00},
{  VirgoA, 10.80, 70.00},
{  VirgoA, 10.85, 70.00},
{    TauA, 10.90, 615.00},
{J1229+020, 10.95, 30.00},
{J1229+020, 11.00, 30.00},
{  VirgoA, 11.05, 70.00},
{J1229+020, 11.10, 30.00},
{J1229+020, 11.15, 30.00},
{  VirgoA, 11.20, 70.00},
{J1229+020, 11.25, 30.00},
{J1229+020, 11.30, 30.00},
{J1229+020, 11.35, 30.00},
{  VirgoA, 11.40, 70.00},
{  VirgoA, 11.45, 70.00},
{J1229+020, 11.50, 30.00},
{  VirgoA, 11.55, 70.00},
{J1229+020, 11.60, 30.00},
{J1229+020, 11.65, 30.00},
{  VirgoA, 11.70, 70.00},
{  VirgoA, 11.75, 70.00},
{J1229+020, 11.80, 30.00},
{J1229+020, 11.85, 30.00},
{J1229+020, 11.90, 30.00},
{J1229+020, 11.95, 30.00},
{J1229+020, 12.00, 30.00},
{J1229+020, 12.05, 30.00},
{  VirgoA, 12.10, 70.00},
{J1229+020, 12.15, 30.00},
{J1229+020, 12.20, 30.00},
{  VirgoA, 12.25, 70.00},
{  VirgoA, 12.30, 70.00},
{  VirgoA, 12.35, 70.00},
{  VirgoA, 12.40, 70.00},
{  VirgoA, 12.45, 70.00},
{  VirgoA, 12.50, 70.00},
{  VirgoA, 12.55, 70.00},
{  VirgoA, 12.60, 70.00},
{  VirgoA, 12.65, 70.00},
{  VirgoA, 12.70, 70.00},
{  VirgoA, 12.75, 70.00},
{  VirgoA, 12.80, 70.00},
{  VirgoA, 12.85, 70.00},
{J1229+020, 12.90, 30.00},
{  VirgoA, 12.95, 70.00},
{J1229+020, 13.00, 30.00},
{J1229+020, 13.05, 30.00},
{J1229+020, 13.10, 30.00},
{  VirgoA, 13.15, 70.00},
{J1229+020, 13.20, 30.00},
{  VirgoA, 13.25, 70.00},
{J1229+020, 13.30, 30.00},
{  VirgoA, 13.35, 70.00},
{  VirgoA, 13.40, 70.00},
{J1229+020, 13.45, 30.00},
{J1229+020, 13.50, 30.00},
{  VirgoA, 13.55, 70.00},
{  VirgoA, 13.60, 70.00},
{  VirgoA, 13.65, 70.00},
{  VirgoA, 13.70, 70.00},
{    CygA, 13.75, 365.00},
{    CygA, 13.80, 365.00},
{    CygA, 13.85, 365.00},
{J1229+020, 13.90, 30.00},
{    CygA, 13.95, 365.00},
{    CygA, 14.00, 365.00},
{  VirgoA, 14.05, 70.00},
{  VirgoA, 14.10, 70.00},
{    CygA, 14.15, 365.00},
{  VirgoA, 14.20, 70.00},
{J1229+020, 14.25, 30.00},
{J1229+020, 14.30, 30.00},
{  VirgoA, 14.35, 70.00},
{  VirgoA, 14.40, 70.00},
{    CygA, 14.45, 365.00},
{    CygA, 14.50, 365.00},
{J1229+020, 14.55, 30.00},
{    CygA, 14.60, 365.00},
{J1229+020, 14.65, 30.00},
{J1229+020, 14.70, 30.00},
{J1229+020, 14.75, 30.00},
{  VirgoA, 14.80, 70.00},
{J1229+020, 14.85, 30.00},
{  VirgoA, 14.90, 70.00},
{J1229+020, 14.95, 30.00},
{J1229+020, 15.00, 30.00},
{  VirgoA, 15.05, 70.00},
{    CygA, 15.10, 365.00},
{  VirgoA, 15.15, 70.00},
{J1229+020, 15.20, 30.00},
{J1229+020, 15.25, 30.00},
{J1229+020, 15.30, 30.00},
{  VirgoA, 15.35, 70.00},
{  VirgoA, 15.40, 70.00},
{    CygA, 15.45, 365.00},
{    CygA, 15.50, 365.00},
{J1229+020, 15.55, 30.00},
{  VirgoA, 15.60, 70.00},
{J1229+020, 15.65, 30.00},
{  VirgoA, 15.70, 70.00},
{  VirgoA, 15.75, 70.00},
{    CygA, 15.80, 365.00},
{  VirgoA, 15.85, 70.00},
{    CygA, 15.90, 365.00},
{    CasA, 15.95, 670.00},
{J1229+020, 16.00, 30.00},
{    CasA, 16.05, 670.00},
{    CasA, 16.10, 670.00},
{    CasA, 16.15, 670.00},
{    CasA, 16.20, 670.00},
{    CygA, 16.25, 365.00},
{    CygA, 16.30, 365.00},
{    CygA, 16.35, 365.00},
{J1229+020, 16.40, 30.00},
{J1229+020, 16.45, 30.00},
{J1229+020, 16.50, 30.00},
{  VirgoA, 16.55, 70.00},
{    CygA, 16.60, 365.00},
{    CasA, 16.65, 670.00},
{    CasA, 16.70, 670.00},
{    CygA, 16.75, 365.00},
{  VirgoA, 16.80, 70.00},
{    CygA, 16.85, 365.00},
{    CygA, 16.90, 365.00},
{  VirgoA, 16.95, 70.00},
{  VirgoA, 17.00, 70.00},
{    CasA, 17.05, 670.00},
{    CygA, 17.10, 365.00},
{    CasA, 17.15, 670.00},
{  VirgoA, 17.20, 70.00},
{    CygA, 17.25, 365.00},
{  VirgoA, 17.30, 70.00},
{    CasA, 17.35, 670.00},
{    CasA, 17.40, 670.00},
{    CasA, 17.45, 670.00},
{    CasA, 17.50, 670.00},
{    CasA, 17.55, 670.00},
{    CasA, 17.60, 670.00},
{    CygA, 17.65, 365.00},
{    CygA, 17.70, 365.00},
{    CasA, 17.75, 670.00},
{    CasA, 17.80, 670.00},
{    CasA, 17.85, 670.00},
{    CygA, 17.90, 365.00},
{    CygA, 17.95, 365.00},
{    CasA, 18.00, 670.00},
{    CasA, 18.05, 670.00},
{    CasA, 18.10, 670.00},
{    CygA, 18.15, 365.00},
{    CasA, 18.20, 670.00},
{    CasA, 18.25, 670.00},
{    CasA, 18.30, 670.00},
{    CasA, 18.35, 670.00},
{    CygA, 18.40, 365.00},
{    CygA, 18.45, 365.00},
{    CygA, 18.50, 365.00},
{    CygA, 18.55, 365.00},
{    CasA, 18.60, 670.00},
{    CasA, 18.65, 670.00},
{    CygA, 18.70, 365.00},
{    CasA, 18.75, 670.00},
{    CygA, 18.80, 365.00},
{    CygA, 18.85, 365.00},
{    CygA, 18.90, 365.00},
{    CygA, 18.95, 365.00},
{    CasA, 19.00, 670.00},
{    CasA, 19.05, 670.00},
{    CygA, 19.10, 365.00},
{    CasA, 19.15, 670.00},
{    CasA, 19.20, 670.00},
{    CygA, 19.25, 365.00},
{    CygA, 19.30, 365.00},
{    CygA, 19.35, 365.00},
{    CygA, 19.40, 365.00},
{    CasA, 19.45, 670.00},
{    CygA, 19.50, 365.00},
{    CygA, 19.55, 365.00},
{    CygA, 19.60, 365.00},
{    CygA, 19.65, 365.00},
{    CygA, 19.70, 365.00},
{    CygA, 19.75, 365.00},
{    CasA, 19.80, 670.00},
{    CasA, 19.85, 670.00},
{    CygA, 19.90, 365.00},
{    CasA, 19.95, 670.00},
{    CygA, 20.00, 365.00},
{    CasA, 20.05, 670.00},
{    CygA, 20.10, 365.00},
{    CygA, 20.15, 365.00},
{    CygA, 20.20, 365.00},
{    CasA, 20.25, 670.00},
{    CasA, 20.30, 670.00},
{    CygA, 20.35, 365.00},
{    CygA, 20.40, 365.00},
{    CasA, 20.45, 670.00},
{    CasA, 20.50, 670.00},
{    CygA, 20.55, 365.00},
{    CygA, 20.60, 365.00},
{    CasA, 20.65, 670.00},
{    CasA, 20.70, 670.00},
{    CasA, 20.75, 670.00},
{    CasA, 20.80, 670.00},
{    CasA, 20.85, 670.00},
{    CygA, 20.90, 365.00},
{    CygA, 20.95, 365.00},
{    CygA, 21.00, 365.00},
{    CasA, 21.05, 670.00},
{    CasA, 21.10, 670.00},
{    CygA, 21.15, 365.00},
{    CasA, 21.20, 670.00},
{J0319+415, 21.25, 23.30},
{J0319+415, 21.30, 23.30},
{    CasA, 21.35, 670.00},
{    CygA, 21.40, 365.00},
{J0319+415, 21.45, 23.30},
{J0319+415, 21.50, 23.30},
{    CasA, 21.55, 670.00},
{J0319+415, 21.60, 23.30},
{    CygA, 21.65, 365.00},
{    CasA, 21.70, 670.00},
{    CasA, 21.75, 670.00},
{    CasA, 21.80, 670.00},
{J0319+415, 21.85, 23.30},
{J0319+415, 21.90, 23.30},
{J0319+415, 21.95, 23.30},
{    CasA, 22.00, 670.00},
{J0319+415, 22.05, 23.30},
{    CygA, 22.10, 365.00},
{    CasA, 22.15, 670.00},
{    CasA, 22.20, 670.00},
{    CygA, 22.25, 365.00},
{    CasA, 22.30, 670.00},
{    CygA, 22.35, 365.00},
{    CasA, 22.40, 670.00},
{    CasA, 22.45, 670.00},
{    CygA, 22.50, 365.00},
{    CygA, 22.55, 365.00},
{    CygA, 22.60, 365.00},
{J0319+415, 22.65, 23.30},
{J0319+415, 22.70, 23.30},
{J0319+415, 22.75, 23.30},
{    CasA, 22.80, 670.00},
{J0319+415, 22.85, 23.30},
{J0319+415, 22.90, 23.30},
{    CasA, 22.95, 670.00},
{J0319+415, 23.00, 23.30},
{    CygA, 23.05, 365.00},
{    CygA, 23.10, 365.00},
{    CygA, 23.15, 365.00},
{    CasA, 23.20, 670.00},
{J0319+415, 23.25, 23.30},
{    CasA, 23.30, 670.00},
{    CygA, 23.35, 365.00},
{    CygA, 23.40, 365.00},
{    CasA, 23.45, 670.00},
{J0319+415, 23.50, 23.30},
{J0319+415, 23.55, 23.30},
{J0319+415, 23.60, 23.30},
{J0319+415, 23.65, 23.30},
{    CasA, 23.70, 670.00},
{J0319+415, 23.75, 23.30},
{    CygA, 23.80, 365.00},
{J0319+415, 23.85, 23.30},
{    CygA, 23.90, 365.00},
{    CygA, 23.95, 365.00}
}


# start observation
init_obs("Beginning schedule: radio_pointing.sch")

# do a sky dip
#do_sky_dip

# Set archiving to the correct mode
mark remove, all

#-----------------------------------------------------------------------
# Main loop -- track the next source,do a raster, move on
#-----------------------------------------------------------------------

log "Waiting for lststart..."
until $after($lststart,lst) 

  # Go through list
  foreach(Source source) $sources {

    # Break if past lststop
    if $time(lst)>$lststop {
       log "lststop reached"
       break
    }   

    # Skip sources whose lst has passed
    if $time(lst)>$source.lst  {
       log "too late for source",$source.name
    }

    # Do a cross on next available source
    if $time(lst)<$source.lst {

      # Track Source
      until $after($source.lst,lst) # make sure we're not early
      log "Doing cross on ",$source.name	
      do_point_cross $source.name

      zeroScanOffsets   	# zero offsets
    }
  }

# do a sky dip
#do_sky_dip

terminate("Finished schedule: radio_pointing.sch")

