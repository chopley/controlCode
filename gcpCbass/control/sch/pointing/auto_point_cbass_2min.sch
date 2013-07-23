(Count npoints, Time lststart, Time lststop) 
# number of points to record on each star, start time, stop time.


# allotting three minutes for each star, that's ~3 reps plus slew time.

group Source {
	Source name,
	Time lst}

listof Source sources = {
{  DelSge, 0.00}, #  269.74  32.23  3.82
{  GamSge, 0.03}, #  268.99  35.01  3.47
{  AlpDel, 0.07}, #  258.33  41.13  3.77
{  BetDel, 0.10}, #  257.32  39.95  3.63
{  EtaPeg, 0.13}, #  252.66  72.64  2.94
{  IotPeg, 0.17}, #  252.21  63.31  3.76
{  AlpEqu, 0.20}, #  239.91  40.91  3.92
{  EpsPeg, 0.23}, #  237.18  49.10  2.39
{  BetPeg, 0.27}, #  237.01  75.20  2.42
{   MuPeg, 0.30}, #  235.29  70.57  3.48
{  LamPeg, 0.33}, #  234.74  69.28  3.95
{  BetAqr, 0.37}, #  227.24  35.13  2.91
{  ThePeg, 0.40}, #  225.86  50.31  3.53
{  AlpAqr, 0.43}, #  221.88  44.28  2.96
{  ZetPeg, 0.47}, #  219.06  58.42  3.40
{  GamAqr, 0.50}, #  216.29  45.35  3.84
{  AlpPeg, 0.53}, #  213.01  64.99  2.49
{  LamAqr, 0.57}, #  203.01  42.71  3.74
{  DelAqr, 0.60}, #  199.29  34.98  3.27
{  GamPsc, 0.63}, #  198.81  54.87  3.69
{   88Aqr, 0.67}, #  193.71  30.62  3.66
{   98Aqr, 0.70}, #  190.29  32.24  3.97
{  IotCet, 0.73}, #  173.33  43.95  3.56
{  GamPeg, 0.77}, #  171.45  67.99  2.83
{  BetCet, 0.80}, #  167.46  34.06  2.04
{  AlpAnd, 0.83}, #  166.89  81.90  2.06
{  EtaCet, 0.87}, #  157.69  40.15  3.45
{  TheCet, 0.90}, #  152.09  40.72  3.60
{  TauCet, 0.93}, #  150.30  31.66  3.50
{  ZetCet, 0.97}, #  145.41  35.92  3.73
{  EtaPsc, 1.00}, #  130.79  60.33  3.62
{  DelAnd, 1.03}, #  124.13  79.80  3.27
{  GamCet, 1.07}, #  122.11  39.59  3.47
{  AlpCet, 1.10}, #  117.02  36.91  2.53
{  BetAri, 1.13}, #  115.09  60.33  2.64
{  OmiTau, 1.17}, #  107.80  36.06  3.60
{  AlpAri, 1.20}, #  107.38  59.57  2.00
{   XiTau, 1.23}, #  106.67  36.06  3.74
{  AlpTri, 1.27}, #   99.20  65.34  3.41
{  LamTau, 1.30}, #   98.03  31.20  3.47
{   41Ari, 1.33}, #   92.73  53.04  3.63
{  BetAnd, 1.37}, #   90.38  75.90  2.06
{   17Tau, 1.40}, #   88.43  40.63  3.70
{  EtaTau, 1.43}, #   88.06  40.11  2.87
{   20Tau, 1.47}, #   87.99  40.56  3.87
{   27Tau, 1.50}, #   87.87  39.75  3.63
{  BetTri, 1.53}, #   84.59  63.85  3.00
{  OmiPer, 1.57}, #   78.50  44.29  3.83
{   MuAnd, 1.60}, #   78.07  78.69  3.87
{  ZetPer, 1.63}, #   77.91  42.21  2.85
{  RhoPer, 1.67}, #   72.55  53.83  3.39
{  IotAur, 1.70}, #   69.57  30.71  2.69
{  BetPer, 1.73}, #   68.86  53.66  2.12
{ Gam1And, 1.77}, #   67.66  65.71  2.26
{  EpsPer, 1.80}, #   67.06  44.24  2.89
{   NuPer, 1.83}, #   64.33  47.18  3.77
{  KapPer, 1.87}, #   62.25  53.89  3.80
{  ZetAur, 1.90}, #   60.56  33.02  3.75
{  EtaAur, 1.93}, #   60.04  32.38  3.17
{  EpsAur, 1.97}, #   57.54  34.13  2.99
{  DelPer, 2.00}, #   56.76  48.41  3.01
{  AlpAur, 2.03}, #   53.99  32.49  0.08
{  AlpPer, 2.07}, #   53.82  51.58  1.79
{   51And, 2.10}, #   49.10  68.74  3.57
{  TauPer, 2.13}, #   48.27  56.03  3.95
{  GamPer, 2.17}, #   47.43  54.35  2.93
{  EtaPer, 2.20}, #   42.61  55.85  3.76
{  EpsCas, 2.23}, #   23.80  58.25  3.38
{  DelCas, 2.27}, #   23.62  63.05  2.68
{  ZetCas, 2.30}, #   17.77  71.94  3.66
{  EtaCas, 2.33}, #   17.34  67.67  3.44
{  GamCas, 2.37}, #   16.24  64.64  2.47
{  AlpCas, 2.40}, #   15.93  69.32  2.23
{   50Cas, 2.43}, #   14.35  51.33  3.98
{  BetCas, 2.47}, #    3.11  67.80  2.27
{  AlpUMi, 2.50}, #    0.57  37.58  2.02
{  GamCep, 2.53}, #   -1.69  49.31  3.21
{  IotCep, 2.57}, #  -13.72  59.08  3.52
{  BetCep, 2.60}, #  -18.94  51.03  3.23
{  ChiDra, 2.63}, #  -21.61  36.62  3.57
{  LamAnd, 2.67}, #  -22.04  79.66  3.82
{  EpsDra, 2.70}, #  -24.52  43.56  3.83
{  DelCep, 2.73}, #  -27.43  63.97  3.75
{  DelDra, 2.77}, #  -28.42  40.63  3.07
{  AlpCep, 2.80}, #  -30.92  54.53  2.44
{  ZetCep, 2.83}, #  -31.32  62.31  3.35
{  EtaCep, 2.87}, #  -34.46  51.21  3.43
{  AlpLac, 2.90}, #  -43.05  69.31  3.77
{  KapCyg, 2.93}, #  -47.16  39.81  3.77
{ Iot2Cyg, 2.97}, #  -49.74  41.39  3.79
{   32Cyg, 3.00}, #  -56.83  48.13  3.98
{   31Cyg, 3.03}, #  -58.21  47.70  3.79
{  DelCyg, 3.07}, #  -59.13  42.51  2.87
{  OmiAnd, 3.10}, #  -60.00  77.64  3.62
{  AlpCyg, 3.13}, #  -61.32  52.33  1.25
{   XiCyg, 3.17}, #  -64.18  56.40  3.72
{  GamCyg, 3.20}, #  -68.15  48.02  2.20
{   NuCyg, 3.23}, #  -68.77  54.69  3.94
{  EtaCyg, 3.27}, #  -72.92  41.57  3.89
{  TauCyg, 3.30}, #  -75.32  57.50  3.72
{ Bet1Cyg, 3.33}, #  -78.25  33.65  3.08
{  EpsCyg, 3.37}, #  -79.49  50.86  2.46
{  ZetCyg, 3.40}, #  -88.59  54.85  3.20
{  GamPeg, 3.43}, #  255.71  42.57  2.83
{  AlpTri, 3.47}, #  255.24  69.67  3.41
{  EtaPsc, 3.50}, #  236.54  56.98  3.62
{  BetAri, 3.53}, #  236.34  64.73  2.64
{  AlpAri, 3.57}, #  236.03  68.66  2.00
{  EtaCet, 3.60}, #  220.89  33.12  3.45
{   41Ari, 3.63}, #  218.84  77.90  3.63
{  TheCet, 3.67}, #  218.17  36.80  3.60
{  ZetCet, 3.70}, #  209.34  37.92  3.73
{  TauCet, 3.73}, #  208.63  32.06  3.50
{  GamCet, 3.77}, #  197.89  54.96  3.47
{  AlpCet, 3.80}, #  189.90  56.71  2.53
{  EtaEri, 3.83}, #  189.43  43.65  3.89
{ Tau4Eri, 3.87}, #  181.22  31.23  3.69
{  OmiTau, 3.90}, #  179.57  62.03  3.60
{   XiTau, 3.93}, #  178.30  62.72  3.74
{  EpsEri, 3.97}, #  176.96  43.49  3.73
{  DelEri, 4.00}, #  173.51  43.02  3.54
{  GamEri, 4.03}, #  169.35  38.86  2.95
{   NuTau, 4.07}, #  161.54  57.73  3.91
{   17Tau, 4.10}, #  159.39  76.36  3.70
{  LamTau, 4.13}, #  159.09  64.15  3.47
{   20Tau, 4.17}, #  158.17  76.53  3.87
{   53Eri, 4.20}, #  157.67  35.80  3.87
{  EtaTau, 4.23}, #  157.03  76.16  2.87
{   27Tau, 4.27}, #  155.62  75.97  3.63
{   NuEri, 4.30}, #  153.37  46.28  3.93
{   MuLep, 4.33}, #  149.19  30.93  3.31
{  GamTau, 4.37}, #  146.24  65.31  3.65
{  BetEri, 4.40}, #  144.43  41.44  2.79
{  BetOri, 4.43}, #  144.42  37.91  0.12
{  Pi5Ori, 4.47}, #  143.67  49.68  3.72
{  TauOri, 4.50}, #  142.74  38.74  3.60
{ Del1Tau, 4.53}, #  142.34  66.61  3.76
{  Pi4Ori, 4.57}, #  142.29  52.82  3.69
{  Pi3Ori, 4.60}, #  141.66  54.17  3.19
{ The2Tau, 4.63}, #  141.58  64.47  3.40
{ The1Tau, 4.67}, #  141.51  64.56  3.84
{  EtaOri, 4.70}, #  137.83  41.68  3.36
{  AlpTau, 4.73}, #  137.40  64.07  0.85
{  IotOri, 4.77}, #  137.31  37.25  2.77
{  EpsTau, 4.80}, #  137.18  67.26  3.53
{  KapOri, 4.83}, #  136.72  32.43  2.06
{  DelOri, 4.87}, #  134.15  42.39  2.23
{  SigOri, 4.90}, #  134.08  39.53  3.81
{  EpsOri, 4.93}, #  133.70  41.04  1.70
{  GamOri, 4.97}, #  130.51  48.76  1.64
{  GamMon, 5.00}, #  128.00  31.17  3.98
{  ZetPer, 5.03}, #  127.29  81.96  2.85
{  LamOri, 5.07}, #  124.06  49.92  3.54
{  AlpOri, 5.10}, #  121.24  44.70  0.50
{  ZetTau, 5.13}, #  109.36  57.02  3.00
{   XiGem, 5.17}, #  104.66  39.15  3.36
{  GamGem, 5.20}, #  102.39  42.71  1.93
{  EtaGem, 5.23}, #   99.31  50.57  3.28
{  BetTau, 5.27}, #   99.22  63.10  1.65
{   MuGem, 5.30}, #   97.80  48.98  2.88
{  LamGem, 5.33}, #   95.16  34.83  3.58
{  IotAur, 5.37}, #   94.49  70.64  2.69
{  ZetGem, 5.40}, #   93.16  39.79  3.79
{  EpsGem, 5.43}, #   90.92  46.10  2.98
{  DelGem, 5.47}, #   89.15  37.33  3.53
{  KapGem, 5.50}, #   83.15  33.69  3.57
{  IotGem, 5.53}, #   81.88  39.00  3.79
{  BetGem, 5.57}, #   79.17  35.24  1.14
{  TheGem, 5.60}, #   77.93  47.92  3.60
{  TheAur, 5.63}, #   77.59  59.18  2.62
{   NuAur, 5.67}, #   74.29  61.09  3.97
{  ZetAur, 5.70}, #   70.32  70.52  3.75
{  EtaAur, 5.73}, #   70.00  69.75  3.17
{  EpsPer, 5.77}, #   62.96  82.73  2.89
{  BetAur, 5.80}, #   62.36  59.90  1.90
{  EpsAur, 5.83}, #   62.17  70.23  2.99
{  AlpAur, 5.87}, #   57.88  67.22  0.08
{  IotUMa, 5.90}, #   50.37  30.34  3.14
{  DelAur, 5.93}, #   44.34  58.38  3.72
{  OmiUMa, 5.97}, #   37.14  38.00  3.36
{   23UMa, 6.00}, #   32.13  31.64  3.67
{  DelPer, 6.03}, #   16.38  78.67  3.01
{  AlpPer, 6.07}, #    0.23  77.14  1.79
{  AlpUMi, 6.10}, #   -0.21  37.72  2.02
{  GamPer, 6.13}, #   -9.89  73.16  2.93
{   50Cas, 6.17}, #   -9.98  53.15  3.98
{  EtaPer, 6.20}, #  -13.93  70.29  3.76
{  GamCep, 6.23}, #  -14.10  43.09  3.21
{  TauPer, 6.27}, #  -15.91  73.40  3.95
{  KapPer, 6.30}, #  -18.05  81.68  3.80
{  EpsCas, 6.33}, #  -19.82  60.10  3.38
{  BetCep, 6.37}, #  -23.95  34.95  3.23
{  DelCas, 6.40}, #  -29.46  60.15  2.68
{  IotCep, 6.43}, #  -30.33  41.94  3.52
{  GamCas, 6.47}, #  -32.46  56.89  2.47
{  AlpCep, 6.50}, #  -32.75  31.70  2.44
{  EtaCas, 6.53}, #  -38.04  57.27  3.44
{  BetCas, 6.57}, #  -38.64  51.90  2.27
{  ZetCep, 6.60}, #  -40.02  36.65  3.35
{  DelCep, 6.63}, #  -40.34  39.05  3.75
{  AlpCas, 6.67}, #  -40.98  56.62  2.23
{  ZetCas, 6.70}, #  -45.87  56.85  3.66
{   51And, 6.73}, #  -50.47  67.52  3.57
{  AlpLac, 6.77}, #  -50.57  37.63  3.77
{  LamAnd, 6.80}, #  -58.63  47.65  3.82
{  OmiAnd, 6.83}, #  -62.29  40.53  3.62
{ Gam1And, 6.87}, #  -64.68  73.74  2.26
{  EtaPeg, 6.90}, #  -74.45  32.42  2.94
{   MuAnd, 6.93}, #  -75.64  61.05  3.87
{  BetPeg, 6.97}, #  -79.24  35.48  2.42
{   MuPeg, 7.00}, #  -81.14  31.11  3.48
{  BetAnd, 7.03}, #  -82.71  63.06  2.06
{  AlpAnd, 7.07}, #  -86.30  48.71  2.06
{  DelAnd, 7.10}, #  -87.89  55.57  3.27
{  BetAnd, 7.13}, #  -82.71  63.06  2.06
{  AlpAnd, 7.17}, #  -86.30  48.71  2.06
{   27Tau, 7.20}, #  267.23  46.20  3.63
{   XiTau, 7.23}, #  255.52  33.81  3.74
{  BetTau, 7.27}, #  255.35  67.51  1.65
{  OmiTau, 7.30}, #  255.30  32.92  3.60
{  EpsTau, 7.33}, #  253.52  51.36  3.53
{ Del1Tau, 7.37}, #  252.70  49.30  3.76
{  LamTau, 7.40}, #  251.74  41.96  3.47
{  GamTau, 7.43}, #  251.11  47.53  3.65
{ The1Tau, 7.47}, #  249.52  49.39  3.84
{ The2Tau, 7.50}, #  249.39  49.35  3.40
{  AlpTau, 7.53}, #  248.44  51.11  0.85
{   NuTau, 7.57}, #  244.83  38.13  3.91
{  ZetTau, 7.60}, #  236.51  65.13  3.00
{  Pi3Ori, 7.63}, #  234.37  46.87  3.19
{  Pi4Ori, 7.67}, #  232.72  46.06  3.69
{   NuEri, 7.70}, #  229.19  36.73  3.93
{  Pi5Ori, 7.73}, #  229.10  44.05  3.72
{  GamOri, 7.77}, #  223.08  51.69  1.64
{  LamOri, 7.80}, #  222.92  56.04  3.54
{  EtaGem, 7.83}, #  220.83  71.79  3.28
{  BetEri, 7.87}, #  219.66  39.70  2.79
{  EtaOri, 7.90}, #  216.60  44.09  3.36
{  BetOri, 7.93}, #  215.84  37.80  0.12
{  TauOri, 7.97}, #  215.80  39.36  3.60
{   MuGem, 8.00}, #  215.66  72.79  2.88
{  DelOri, 8.03}, #  215.64  46.82  2.23
{  EpsOri, 8.07}, #  213.68  46.50  1.70
{  AlpOri, 8.10}, #  212.94  56.31  0.50
{  SigOri, 8.13}, #  211.99  45.51  3.81
{   MuLep, 8.17}, #  211.86  30.50  3.31
{  IotOri, 8.20}, #  211.10  42.16  2.77
{  AlpLep, 8.23}, #  206.06  30.93  2.58
{  KapOri, 8.27}, #  205.46  39.85  2.06
{  ZetLep, 8.30}, #  203.50  34.93  3.55
{  EpsGem, 8.33}, #  203.19  77.23  2.98
{  EtaLep, 8.37}, #  201.08  36.27  3.71
{  GamGem, 8.40}, #  198.77  68.47  1.93
{  GamMon, 8.43}, #  198.16  45.10  3.98
{  BetCMa, 8.47}, #  192.46  34.10  1.98
{   XiGem, 8.50}, #  192.22  65.45  3.36
{  Nu2CMa, 8.53}, #  188.29  33.32  3.95
{  AlpCMa, 8.57}, #  186.18  36.06 -1.46
{  ZetGem, 8.60}, #  181.57  73.57  3.79
{  LamGem, 8.63}, #  171.75  69.37  3.58
{  BetCMi, 8.67}, #  169.20  60.89  2.90
{  AlpMon, 8.70}, #  168.13  42.72  3.93
{  DelGem, 8.73}, #  167.52  74.67  3.53
{  AlpCMi, 8.77}, #  164.51  57.32  0.38
{  IotGem, 8.80}, #  154.29  79.90  3.79
{  BetCnc, 8.83}, #  145.73  57.93  3.52
{  KapGem, 8.87}, #  144.15  74.95  3.57
{  EpsHya, 8.90}, #  136.89  51.76  3.38
{  AlpHya, 8.93}, #  136.58  33.57  1.98
{  BetGem, 8.97}, #  134.52  77.80  1.14
{  ZetHya, 9.00}, #  134.50  50.17  3.11
{  TheHya, 9.03}, #  131.95  44.46  3.88
{  IotHya, 9.07}, #  128.07  37.82  3.91
{  DelCnc, 9.10}, #  124.33  61.30  3.94
{  OmiLeo, 9.13}, #  117.77  45.78  3.52
{  AlpLeo, 9.17}, #  109.42  42.18  1.35
{  RhoLeo, 9.20}, #  107.14  35.86  3.85
{  EtaLeo, 9.23}, #  104.39  45.33  3.52
{  EpsLeo, 9.27}, #   99.67  53.40  2.98
{ Gam2Leo, 9.30}, #   98.41  44.60  3.80
{ Gam1Leo, 9.33}, #   98.40  44.60  2.61
{   MuLeo, 9.37}, #   95.13  53.12  3.88
{  ZetLeo, 9.40}, #   94.59  47.11  3.44
{  TheLeo, 9.43}, #   93.99  31.39  3.34
{  DelLeo, 9.47}, #   88.92  34.18  2.56
{  AlpLyn, 9.50}, #   85.23  62.60  3.13
{   38Lyn, 9.53}, #   80.26  63.58  3.82
{   46LMi, 9.57}, #   75.67  44.41  3.83
{   NuUMa, 9.60}, #   74.44  39.13  3.48
{   MuUMa, 9.63}, #   67.54  52.24  3.05
{  LamUMa, 9.67}, #   65.46  53.40  3.45
{  PsiUMa, 9.70}, #   60.61  44.32  3.01
{  KapUMa, 9.73}, #   55.55  66.12  3.60
{  ChiUMa, 9.77}, #   54.33  38.99  3.71
{  IotUMa, 9.80}, #   52.91  66.59  3.14
{  TheUMa, 9.83}, #   48.55  60.36  3.17
{  GamUMa, 9.87}, #   46.55  39.16  2.44
{  BetUMa, 9.90}, #   43.88  46.82  2.37
{  DelUMa, 9.93}, #   41.59  36.87  3.31
{  EpsUMa, 9.97}, #   40.95  31.47  1.77
{  UpsUMa, 10.00}, #   36.88  55.59  3.80
{  AlpUMa, 10.03}, #   36.11  46.25  1.79
{   23UMa, 10.07}, #   28.62  55.87  3.67
{  LamDra, 10.10}, #   26.03  42.54  3.84
{  KapDra, 10.13}, #   25.41  37.14  3.87
{  OmiUMa, 10.17}, #   22.62  62.80  3.36
{  AlpUMi, 10.20}, #   -0.86  37.27  2.02
{  GamCep, 10.23}, #  -13.52  31.65  3.21
{   50Cas, 10.27}, #  -22.22  39.31  3.98
{  DelAur, 10.30}, #  -28.17  69.29  3.72
{  EpsCas, 10.33}, #  -33.32  37.85  3.38
{  GamCas, 10.37}, #  -34.60  30.60  2.47
{  DelCas, 10.40}, #  -36.54  33.82  2.68
{  EtaPer, 10.43}, #  -44.48  44.09  3.76
{  GamPer, 10.47}, #  -48.03  45.98  2.93
{  TauPer, 10.50}, #  -48.90  44.35  3.95
{   51And, 10.53}, #  -50.27  31.67  3.57
{  BetAur, 10.57}, #  -52.37  75.19  1.90
{  AlpPer, 10.60}, #  -53.67  48.79  1.79
{  DelPer, 10.63}, #  -57.17  51.73  3.01
{  AlpAur, 10.67}, #  -57.51  67.79  0.08
{ Gam1And, 10.70}, #  -59.21  33.56  2.26
{  KapPer, 10.73}, #  -60.49  45.64  3.80
{  EpsAur, 10.77}, #  -64.05  65.59  2.99
{   NuPer, 10.80}, #  -65.59  51.59  3.77
{  BetPer, 10.83}, #  -65.79  44.51  2.12
{  BetTri, 10.87}, #  -67.71  31.61  3.00
{  RhoPer, 10.90}, #  -68.38  43.36  3.39
{  EtaAur, 10.93}, #  -70.32  66.56  3.17
{  EpsPer, 10.97}, #  -70.42  53.50  2.89
{  ZetAur, 11.00}, #  -70.71  65.80  3.75
{   NuAur, 11.03}, #  -75.96  75.21  3.97
{   41Ari, 11.07}, #  -80.67  35.93  3.63
{  OmiPer, 11.10}, #  -81.06  48.73  3.83
{  ZetPer, 11.13}, #  -82.80  50.53  2.85
{  TheAur, 11.17}, #  -84.06  76.80  2.62
{  IotAur, 11.20}, #  -88.61  63.46  2.69
{  KapGem, 11.23}, #  269.60  43.87  3.57
{  LamGem, 11.26}, #  265.10  34.50  3.58
{  BetCMi, 11.30}, #  255.61  31.41  2.90
{  AlpCMi, 11.33}, #  250.58  31.77  0.38
{  DelCnc, 11.36}, #  250.29  52.31  3.94
{  BetCnc, 11.40}, #  246.66  41.31  3.52
{   MuLeo, 11.43}, #  243.20  69.59  3.88
{  EpsLeo, 11.46}, #  241.15  66.97  2.98
{  EpsHya, 11.50}, #  236.75  44.68  3.38
{  ZetHya, 11.53}, #  234.03  45.74  3.11
{  ZetLeo, 11.56}, #  226.10  71.69  3.44
{  TheHya, 11.60}, #  225.48  45.75  3.88
{  OmiLeo, 11.63}, #  223.53  55.76  3.52
{  EtaLeo, 11.66}, #  220.39  65.00  3.52
{ Gam1Leo, 11.70}, #  218.07  69.21  2.61
{ Gam2Leo, 11.73}, #  218.07  69.21  3.80
{  IotHya, 11.76}, #  215.06  46.08  3.91
{  AlpLeo, 11.80}, #  214.60  60.98  1.35
{  AlpHya, 11.83}, #  214.11  38.00  1.98
{  RhoLeo, 11.86}, #  201.09  60.79  3.85
{  LamHya, 11.90}, #  199.86  38.45  3.61
{   MuHya, 11.93}, #  193.97  35.00  3.81
{   NuHya, 11.96}, #  187.23  36.50  3.11
{  DelLeo, 12.00}, #  179.75  73.52  2.56
{  TheLeo, 12.03}, #  179.71  68.43  3.34
{  DelCrt, 12.06}, #  178.30  38.20  3.56
{  IotLeo, 12.10}, #  174.43  63.43  3.94
{  BetVir, 12.13}, #  164.28  53.75  3.61
{  GamCrv, 12.16}, #  162.20  33.53  2.59
{  BetLeo, 12.20}, #  158.40  66.24  2.14
{  DelCrv, 12.23}, #  157.98  33.56  2.95
{  EtaVir, 12.26}, #  154.12  49.34  3.89
{  GamVir, 12.30}, #  147.11  46.49  3.68
{  AlpVir, 12.33}, #  140.72  32.80  0.98
{  DelVir, 12.36}, #  139.09  49.07  3.38
{  ZetVir, 12.40}, #  130.92  40.25  3.37
{  EpsVir, 12.43}, #  129.92  54.33  2.83
{  EtaBoo, 12.46}, #  106.71  50.23  2.68
{  AlpBoo, 12.50}, #  101.44  46.62 -0.04
{  EpsBoo, 12.53}, #   86.81  44.74  2.70
{  RhoBoo, 12.56}, #   84.20  48.74  3.58
{  AlpCrB, 12.60}, #   80.64  34.72  2.23
{  GamCrB, 12.63}, #   80.05  32.93  3.84
{  BetCrB, 12.66}, #   78.86  37.16  3.68
{ Alp2CVn, 12.70}, #   78.44  69.79  2.90
{  DelBoo, 12.73}, #   75.32  41.29  3.47
{  GamBoo, 12.76}, #   72.46  51.21  3.03
{  BetBoo, 12.80}, #   67.24  46.13  3.50
{  TauHer, 12.83}, #   54.42  34.34  3.89
{  EtaUMa, 12.86}, #   53.65  59.79  1.86
{  IotDra, 12.90}, #   40.21  44.75  3.29
{  EtaDra, 12.93}, #   36.04  37.67  2.74
{  EpsUMa, 12.96}, #   33.62  64.65  1.77
{  ZetDra, 13.00}, #   29.64  33.76  3.17
{  AlpDra, 13.03}, #   29.00  52.81  3.65
{  ChiUMa, 13.06}, #   26.26  77.70  3.71
{  GamUMi, 13.10}, #   22.29  43.61  3.05
{  DelUMa, 13.13}, #   22.20  67.50  3.31
{  GamUMa, 13.16}, #   19.37  71.93  2.44
{  ChiDra, 13.20}, #   19.24  30.41  3.57
{  BetUMi, 13.23}, #   18.24  44.94  2.08
{  KapDra, 13.26}, #   12.00  55.50  3.87
{  LamDra, 13.30}, #    2.90  57.58  3.84
{  AlpUMi, 13.33}, #   -0.70  36.52  2.02
{  AlpUMa, 13.36}, #   -2.84  65.20  1.79
{  BetUMa, 13.40}, #   -4.97  70.51  2.37
{  PsiUMa, 13.43}, #   -5.63  82.46  3.01
{   23UMa, 13.46}, #  -22.76  59.64  3.67
{  UpsUMa, 13.50}, #  -24.71  64.21  3.80
{  OmiUMa, 13.53}, #  -34.03  55.11  3.36
{  TheUMa, 13.56}, #  -42.45  66.94  3.17
{  DelAur, 13.60}, #  -44.67  35.52  3.72
{  IotUMa, 13.63}, #  -55.05  63.13  3.14
{  BetAur, 13.66}, #  -55.33  32.49  1.90
{  KapUMa, 13.70}, #  -56.67  64.03  3.60
{  LamUMa, 13.73}, #  -57.00  77.65  3.45
{   MuUMa, 13.76}, #  -61.71  79.08  3.05
{  TheGem, 13.80}, #  -72.52  37.84  3.60
{  EpsGem, 13.83}, #  -81.14  32.19  2.98
{   38Lyn, 13.86}, #  -81.74  67.11  3.82
{  IotGem, 13.90}, #  -83.64  41.70  3.79
{  BetGem, 13.93}, #  -85.97  45.69  1.14
{  AlpLyn, 13.96}, #  -88.03  67.05  3.13
{  ZetGem, 14.00}, #  -88.64  33.89  3.79
{  DelGem, 14.03}, #  -89.52  37.81  3.53
{ Gam1Leo, 14.07}, #  266.70  38.59  2.61
{ Gam2Leo, 14.10}, #  266.70  38.59  3.80
{  EtaLeo, 14.14}, #  265.51  34.40  3.52
{  AlpLeo, 14.17}, #  260.65  31.85  1.35
{  DelLeo, 14.20}, #  257.93  49.66  2.56
{  RhoLeo, 14.24}, #  253.73  34.98  3.85
{  TheLeo, 14.27}, #  251.70  46.70  3.34
{  IotLeo, 14.30}, #  244.13  45.30  3.94
{  BetLeo, 14.34}, #  242.25  52.55  2.14
{  BetVir, 14.37}, #  229.02  43.25  3.61
{  EtaVir, 14.40}, #  218.64  45.26  3.89
{  EpsVir, 14.44}, #  212.16  60.45  2.83
{  GamVir, 14.47}, #  211.13  47.05  3.68
{  DelVir, 14.50}, #  209.20  52.89  3.38
{  DelCrv, 14.54}, #  206.91  32.05  2.95
{  AlpVir, 14.57}, #  192.71  40.97  0.98
{  ZetVir, 14.60}, #  191.98  51.79  3.37
{  EtaBoo, 14.64}, #  187.05  71.28  2.68
{  AlpBoo, 14.67}, #  171.21  72.01 -0.04
{ Alp2Lib, 14.70}, #  166.13  35.83  2.75
{   MuVir, 14.74}, #  165.91  46.39  3.88
{  109Vir, 14.77}, #  162.11  53.58  3.72
{  BetLib, 14.80}, #  156.06  40.57  2.61
{  GamLib, 14.84}, #  153.12  33.92  3.91
{   MuSer, 14.87}, #  142.91  42.71  3.53
{  AlpSer, 14.90}, #  137.12  51.86  2.65
{  RhoBoo, 14.94}, #  137.10  81.23  3.58
{  EpsSer, 14.97}, #  136.66  49.34  3.71
{  EpsBoo, 15.00}, #  136.12  76.86  2.70
{  DelOph, 15.04}, #  136.10  39.28  2.74
{  EpsOph, 15.07}, #  135.76  37.90  3.24
{  ZetOph, 15.10}, #  135.13  30.42  2.56
{  LamOph, 15.14}, #  127.30  41.37  3.82
{  BetSer, 15.17}, #  126.87  58.75  3.67
{  GamSer, 15.20}, #  123.14  57.25  3.85
{  KapOph, 15.24}, #  113.99  42.13  3.20
{  AlpCrB, 15.27}, #  111.77  68.29  2.23
{  GamHer, 15.30}, #  111.17  55.06  3.75
{  GamCrB, 15.34}, #  110.08  66.55  3.84
{  BetOph, 15.37}, #  109.10  30.44  2.77
{  BetCrB, 15.40}, #  108.24  70.84  3.68
{  BetHer, 15.44}, #  105.89  54.86  2.77
{ Alp1Her, 15.47}, #  105.21  42.15  3.48
{  AlpOph, 15.50}, #  103.22  37.12  2.08
{  DelBoo, 15.54}, #   98.92  74.99  3.47
{  DelHer, 15.57}, #   92.73  47.77  3.14
{  ZetHer, 15.60}, #   87.69  57.35  2.81
{  EpsHer, 15.64}, #   86.24  53.31  3.92
{  109Her, 15.67}, #   85.99  32.56  3.84
{   MuHer, 15.70}, #   84.50  42.82  3.42
{   XiHer, 15.74}, #   81.20  41.24  3.70
{  OmiHer, 15.77}, #   80.57  39.11  3.83
{   PiHer, 15.80}, #   75.40  52.26  3.16
{  EtaHer, 15.84}, #   74.11  58.91  3.53
{  TheHer, 15.87}, #   71.21  44.51  3.86
{  BetLyr, 15.90}, #   70.56  32.88  3.45
{  GamLyr, 15.94}, #   70.31  30.92  3.24
{  BetBoo, 15.97}, #   68.81  78.25  3.50
{  AlpLyr, 16.00}, #   65.76  37.47  0.03
{  IotHer, 16.04}, #   59.69  49.50  3.80
{  TauHer, 16.07}, #   58.89  63.27  3.89
{  GamDra, 16.10}, #   51.08  47.20  2.23
{  BetDra, 16.14}, #   49.91  51.24  2.79
{ Iot2Cyg, 16.17}, #   47.00  33.14  3.79
{  KapCyg, 16.20}, #   45.82  35.46  3.77
{   XiDra, 16.24}, #   43.12  47.70  3.75
{  EtaDra, 16.27}, #   30.35  57.26  2.74
{  DelDra, 16.30}, #   28.32  38.64  3.07
{  ZetDra, 16.34}, #   28.06  50.91  3.17
{  EpsDra, 16.37}, #   24.54  35.83  3.83
{  IotDra, 16.40}, #   24.35  64.44  3.29
{  ChiDra, 16.44}, #   21.32  42.67  3.57
{  GamUMi, 16.47}, #    9.97  53.81  3.05
{  BetUMi, 16.50}, #    5.18  52.42  2.08
{  AlpDra, 16.54}, #    0.04  62.62  3.65
{  AlpUMi, 16.57}, #   -0.11  36.27  2.02
{  EtaUMa, 16.60}, #  -12.46  77.32  1.86
{  KapDra, 16.64}, #  -13.44  55.02  3.87
{  LamDra, 16.67}, #  -20.61  51.68  3.84
{  EpsUMa, 16.70}, #  -26.36  67.66  1.77
{  DelUMa, 16.74}, #  -33.29  63.06  3.31
{  AlpUMa, 16.77}, #  -33.72  52.83  1.79
{   23UMa, 16.80}, #  -34.56  42.15  3.67
{  OmiUMa, 16.84}, #  -36.23  34.70  3.36
{  UpsUMa, 16.87}, #  -40.10  44.48  3.80
{  BetUMa, 16.90}, #  -42.48  54.15  2.37
{  GamUMa, 16.94}, #  -42.89  62.05  2.44
{  TheUMa, 16.97}, #  -49.77  41.22  3.17
{  IotUMa, 17.00}, #  -52.50  35.06  3.14
{  KapUMa, 17.04}, #  -53.82  35.49  3.60
{  ChiUMa, 17.07}, #  -55.86  62.59  3.71
{  PsiUMa, 17.10}, #  -63.16  56.52  3.01
{  LamUMa, 17.14}, #  -63.74  46.91  3.45
{   MuUMa, 17.17}, #  -66.08  47.55  3.05
{   38Lyn, 17.20}, #  -66.86  34.41  3.82
{  AlpLyn, 17.24}, #  -69.74  33.84  3.13
{ Alp2CVn, 17.27}, #  -79.23  76.46  2.90
{   46LMi, 17.30}, #  -79.42  51.51  3.83
{   MuLeo, 17.34}, #  -82.63  36.25  3.88
{   NuUMa, 17.37}, #  -84.02  56.12  3.48
{  EpsLeo, 17.40}, #  -84.09  33.80  2.98
{  ZetLeo, 17.44}, #  -88.83  39.76  3.44
{  EpsBoo, 17.47}, #  265.86  54.31  2.70
{  EtaBoo, 17.50}, #  263.37  39.94  2.68
{  BetCrB, 17.54}, #  261.37  63.70  3.68
{  AlpBoo, 17.57}, #  260.56  44.52 -0.04
{  AlpCrB, 17.60}, #  254.98  63.92  2.23
{  GamCrB, 17.64}, #  251.97  65.25  3.84
{  ZetHer, 17.67}, #  244.29  78.92  2.81
{  109Vir, 17.70}, #  236.40  38.82  3.72
{  BetSer, 17.74}, #  233.13  58.75  3.67
{   MuVir, 17.77}, #  231.15  32.54  3.88
{  GamSer, 17.80}, #  229.72  60.53  3.85
{  EpsHer, 17.84}, #  225.62  81.62  3.92
{  AlpSer, 17.87}, #  224.17  51.33  2.65
{  GamHer, 17.90}, #  223.60  67.01  3.75
{  BetHer, 17.94}, #  223.02  70.04  2.77
{  BetLib, 17.97}, #  220.34  34.35  2.61
{  EpsSer, 18.00}, #  220.32  50.57  3.71
{   MuSer, 18.04}, #  215.03  43.52  3.53
{  GamLib, 18.07}, #  212.49  31.81  3.91
{  DelOph, 18.10}, #  206.97  45.82  2.74
{  EpsOph, 18.14}, #  205.14  45.24  3.24
{  LamOph, 18.17}, #  204.01  52.58  3.82
{  ZetOph, 18.20}, #  196.71  40.95  2.56
{  KapOph, 18.24}, #  195.95  61.52  3.20
{  DelHer, 18.27}, #  193.95  77.52  3.14
{ Alp1Her, 18.30}, #  188.49  67.19  3.48
{  EtaOph, 18.34}, #  185.38  37.11  2.43
{   XiSer, 18.37}, #  177.15  37.55  3.54
{  AlpOph, 18.40}, #  176.03  65.51  2.08
{  BetOph, 18.44}, #  172.93  57.38  2.77
{  GamOph, 18.47}, #  171.31  55.41  3.75
{   NuOph, 18.50}, #  169.65  42.67  3.34
{   MuSgr, 18.54}, #  167.59  30.96  3.86
{   67Oph, 18.57}, #  165.73  55.12  3.97
{  EtaSer, 18.60}, #  159.86  48.23  3.26
{   72Oph, 18.64}, #  159.66  61.17  3.73
{  AlpSct, 18.67}, #  157.39  42.11  3.85
{   MuHer, 18.70}, #  156.18  79.96  3.42
{  LamAql, 18.74}, #  146.03  42.28  3.44
{   XiHer, 18.77}, #  139.31  80.09  3.70
{  109Her, 18.80}, #  137.89  70.60  3.84
{  DelAql, 18.84}, #  134.50  46.72  3.36
{  OmiHer, 18.87}, #  132.14  78.35  3.83
{  ZetAql, 18.90}, #  130.49  58.31  2.99
{  EtaAql, 18.94}, #  128.75  41.00  3.90
{  TheAql, 18.97}, #  125.58  36.59  3.23
{  BetAql, 19.00}, #  123.24  44.65  3.71
{  AlpAql, 19.04}, #  122.02  47.22  0.77
{  GamAql, 19.07}, #  121.43  49.26  2.72
{  DelSge, 19.10}, #  111.64  54.40  3.82
{  GamSge, 19.14}, #  107.61  52.84  3.47
{  BetDel, 19.17}, #  105.21  42.49  3.63
{  AlpDel, 19.20}, #  103.36  42.87  3.77
{ Bet1Cyg, 19.24}, #  100.44  62.77  3.08
{  GamLyr, 19.27}, #   96.32  70.94  3.24
{  BetLyr, 19.30}, #   96.19  72.91  3.45
{  EtaCyg, 19.34}, #   82.39  60.19  3.89
{  ZetCyg, 19.37}, #   81.08  43.39  3.20
{  IotPeg, 19.40}, #   79.75  30.53  3.76
{  EpsCyg, 19.44}, #   79.04  50.03  2.46
{  AlpLyr, 19.47}, #   77.26  76.34  0.03
{  GamCyg, 19.50}, #   70.80  56.20  2.20
{  TauCyg, 19.54}, #   70.60  45.78  3.72
{   NuCyg, 19.57}, #   67.35  49.84  3.94
{   XiCyg, 19.60}, #   62.78  48.95  3.72
{  DelCyg, 19.64}, #   61.55  63.20  2.87
{  AlpCyg, 19.67}, #   61.45  53.26  1.25
{   31Cyg, 19.70}, #   59.00  58.11  3.79
{   32Cyg, 19.74}, #   57.21  57.75  3.98
{  AlpLac, 19.77}, #   50.00  36.04  3.77
{ Iot2Cyg, 19.80}, #   45.72  64.06  3.79
{  ZetCep, 19.84}, #   40.91  40.60  3.35
{  KapCyg, 19.87}, #   40.32  65.05  3.77
{  DelCep, 19.90}, #   40.17  38.26  3.75
{  AlpCep, 19.94}, #   34.74  46.99  2.44
{  EtaCep, 19.97}, #   34.58  50.94  3.43
{  IotCep, 20.00}, #   29.96  37.20  3.52
{  BetCep, 20.04}, #   23.83  44.42  3.23
{  EpsDra, 20.07}, #   18.30  51.94  3.83
{  DelDra, 20.10}, #   17.40  56.02  3.07
{  GamDra, 20.14}, #   16.92  74.66  2.23
{  GamCep, 20.17}, #   15.22  35.42  3.21
{  IotHer, 20.20}, #   12.26  80.75  3.80
{   XiDra, 20.24}, #   10.00  69.68  3.75
{  ChiDra, 20.27}, #    6.57  53.66  3.57
{  BetDra, 20.30}, #    1.29  74.69  2.79
{  AlpUMi, 20.34}, #    0.64  36.47  2.02
{  ZetDra, 20.37}, #   -4.13  61.15  3.17
{  BetUMi, 20.40}, #  -15.11  48.37  2.08
{  GamUMi, 20.44}, #  -15.35  51.56  3.05
{  EtaDra, 20.47}, #  -17.20  63.51  2.74
{  LamDra, 20.50}, #  -25.37  34.54  3.84
{  KapDra, 20.54}, #  -25.63  39.96  3.87
{  AlpDra, 20.57}, #  -31.13  49.48  3.65
{  IotDra, 20.60}, #  -32.30  60.37  3.29
{  DelUMa, 20.64}, #  -41.45  36.43  3.31
{  EpsUMa, 20.67}, #  -44.05  41.47  1.77
{  GamUMa, 20.70}, #  -44.26  32.53  2.44
{  TauHer, 20.74}, #  -48.44  74.24  3.89
{  EtaUMa, 20.77}, #  -54.51  48.92  1.86
{ Alp2CVn, 20.80}, #  -66.35  37.40  2.90
{  BetBoo, 20.84}, #  -71.78  61.47  3.50
{  GamBoo, 20.87}, #  -74.08  55.45  3.03
{  EtaHer, 20.90}, #  -74.40  80.87  3.53
{  RhoBoo, 20.94}, #  -87.06  53.05  3.58
{  DelBoo, 20.97}, #  -87.78  62.77  3.47
{  OmiHer, 21.00}, #  269.92  53.19  3.83
{  GamLyr, 21.03}, #  269.17  64.90  3.24
{  EtaCyg, 21.07}, #  266.29  77.00  3.89
{ Alp1Her, 21.10}, #  261.21  35.34  3.48
{  109Her, 21.13}, #  256.76  53.03  3.84
{  AlpOph, 21.17}, #  255.71  38.22  2.08
{ Bet1Cyg, 21.20}, #  250.73  69.18  3.08
{  BetOph, 21.23}, #  246.43  34.67  2.77
{   72Oph, 21.27}, #  246.01  42.41  3.73
{  GamOph, 21.30}, #  243.87  34.19  3.75
{   67Oph, 21.33}, #  241.31  36.61  3.97
{  ZetAql, 21.37}, #  235.37  55.55  2.99
{  EtaSer, 21.40}, #  231.73  35.70  3.26
{  DelSge, 21.43}, #  225.69  65.62  3.82
{  AlpSct, 21.47}, #  224.48  33.47  3.85
{  GamSge, 21.50}, #  221.57  67.97  3.47
{  LamAql, 21.53}, #  218.56  40.43  3.44
{  DelAql, 21.57}, #  218.27  49.76  3.36
{  GamAql, 21.60}, #  216.89  58.82  2.72
{  AlpAql, 21.63}, #  213.47  57.80  0.77
{  BetAql, 21.67}, #  209.70  56.04  3.71
{ Rho1Sgr, 21.70}, #  207.35  30.45  3.93
{  EtaAql, 21.73}, #  207.33  50.77  3.90
{  TheAql, 21.77}, #  199.37  50.53  3.23
{  BetDel, 21.80}, #  194.04  67.04  3.63
{  AlpDel, 21.83}, #  193.42  68.43  3.77
{ Alp2Cap, 21.87}, #  193.31  39.48  3.57
{  BetCap, 21.90}, #  191.89  37.41  3.08
{  EpsAqr, 21.93}, #  184.18  43.41  3.77
{  ZetCap, 21.97}, #  172.86  30.26  3.74
{  AlpEqu, 22.00}, #  172.54  58.04  3.92
{  BetAqr, 22.03}, #  168.49  46.80  2.91
{  GamCap, 22.07}, #  168.18  35.51  3.68
{  DelCap, 22.10}, #  166.04  35.73  2.87
{  ZetCyg, 22.13}, #  157.40  82.71  3.20
{  EpsPeg, 22.17}, #  157.01  61.10  2.39
{  AlpAqr, 22.20}, #  154.05  49.69  2.96
{  GamAqr, 22.23}, #  149.12  47.20  3.84
{  ThePeg, 22.27}, #  148.24  55.28  3.53
{  DelAqr, 22.30}, #  147.55  30.67  3.27
{  LamAqr, 22.33}, #  143.47  38.21  3.74
{  ZetPeg, 22.37}, #  132.39  55.27  3.40
{  GamPsc, 22.40}, #  128.70  43.88  3.69
{  IotPeg, 22.43}, #  124.48  71.58  3.76
{  AlpPeg, 22.47}, #  119.90  54.80  2.49
{  LamPeg, 22.50}, #  112.93  63.50  3.95
{   MuPeg, 22.53}, #  110.01  63.46  3.48
{  GamPeg, 22.57}, #  103.81  42.09  2.83
{  EtaPeg, 22.60}, #  100.03  67.59  2.94
{  BetPeg, 22.63}, #   99.93  62.58  2.42
{  AlpAnd, 22.67}, #   87.29  50.15  2.06
{  DelAnd, 22.70}, #   80.92  44.71  3.27
{  BetAnd, 22.73}, #   71.67  40.61  2.06
{   MuAnd, 22.77}, #   69.16  44.01  3.87
{  OmiAnd, 22.80}, #   67.63  66.08  3.62
{  LamAnd, 22.83}, #   59.40  59.48  3.82
{ Gam1And, 22.87}, #   59.06  33.25  2.26
{   51And, 22.90}, #   53.39  39.53  3.57
{  ZetCas, 22.93}, #   47.51  49.57  3.66
{  AlpLac, 22.97}, #   43.65  68.95  3.77
{  AlpCas, 23.00}, #   43.51  48.92  2.23
{  EtaCas, 23.03}, #   41.72  47.67  3.44
{  EtaPer, 23.07}, #   40.86  31.10  3.76
{  DelCas, 23.10}, #   38.41  42.91  2.68
{  BetCas, 23.13}, #   38.35  52.60  2.27
{  GamCas, 23.17}, #   37.58  46.47  2.47
{  EpsCas, 23.20}, #   33.60  39.75  3.38
{  DelCep, 23.23}, #   27.07  64.12  3.75
{  ZetCep, 23.27}, #   23.06  65.83  3.35
{   50Cas, 23.30}, #   22.22  39.25  3.98
{  IotCep, 23.33}, #   19.84  56.79  3.52
{  GamCep, 23.37}, #   11.39  45.99  3.21
{  AlpCep, 23.40}, #    4.92  64.25  2.44
{  BetCep, 23.43}, #    4.28  56.22  3.23
{  AlpUMi, 23.47}, #    0.92  37.09  2.02
{  EtaCep, 23.50}, #   -4.12  65.06  3.43
{  EpsDra, 23.53}, #  -10.56  55.38  3.83
{  ChiDra, 23.57}, #  -16.88  49.20  3.57
{  DelDra, 23.60}, #  -17.81  55.84  3.07
{  BetUMi, 23.63}, #  -19.39  34.76  2.08
{  AlpCyg, 23.67}, #  -21.39  81.02  1.25
{  GamUMi, 23.70}, #  -22.71  36.46  3.05
{  ZetDra, 23.73}, #  -30.28  46.35  3.17
{   32Cyg, 23.77}, #  -33.87  76.52  3.98
{  EtaDra, 23.80}, #  -36.62  41.72  2.74
{   31Cyg, 23.83}, #  -37.79  77.01  3.79
{  IotDra, 23.87}, #  -38.23  34.08  3.29
{  KapCyg, 23.90}, #  -39.21  65.82  3.77
{ Iot2Cyg, 23.93}, #  -40.05  68.32  3.79
{   XiDra, 23.97} #  -41.88  53.49  3.75
}


# Get rid of any legacy signals.
signal/init done
mark remove, all
archive combine=1, filter=false
open grabber

model optical, ptel=0
offset az=0, el=0
sky_offset x=0, y=0
radec_offset ra=0, dec=0
 
 
# Configure the frame grabber 
#setDefaultFrameGrabberChannel chan1
#configureFrameGrabber chan=chan1
#setOpticalCameraRotation angle=180
#configureFrameGrabber flatfield=image
#setOpticalCameraFov fov=11.5, chan=chan1
#addSearchBox 11, 12, 495, 465, true, chan=chan1


# let's first take a flatfield
#slew az=180, el=80
#until $acquired(source)
#takeFlatfield chan=chan1
configureFrameGrabber flatfield=image, chan=chan1
configureFrameGrabber combine=5

# wait for lst start
until $after($lststart,lst)


foreach(Source source) $sources {

	if $time(lst)>$lststop {
		log "lststop reached"
		break
  	}

	if($time(lst)>$source.lst) { 
		log "too late for source ", $source.name
 	} else if($elevation($source.name)>20 & $elevation($source.name)<83) {
	 
		offset az=0, el=0
		track $source.name
  		until $acquired(source)
  		#wait for the antenna to settle
  		until $elapsed > 15s

		# Get an image
  		grabFrame chan=chan1
		until $acquired(grab)
  		until $elapsed > 3s

		# Check if we have a star within 10arcmin


  		if($imstat(snr) > 8 & $peak(xabs) < 00:11:00 & $peak(yabs) < 00:11:00) {
     			print "Found star at ",$peak(x),",",$peak(y)," with snr ",$imstat(snr)
     			center # offset to star
     			until $acquired(source)
     			until $elapsed>5s # always wait for the telescope to settle after center

     			# Now record a few points to sample the seeing disk
     			do Count i=1,$npoints,1 {
				grabFrame chan=chan1
				until $acquired(grab)
				until $elapsed>3s

			if($imstat(snr) > 8 & $peak(xabs) < 00:06:00 & $peak(yabs) < 00:06:00) {
	  			print "Star offset ",$peak(x),",",$peak(y),", snr ",$imstat(snr)
	  			center 
	  			until $elapsed>5s
				mark add, f0
				until $elapsed>3s
				mark remove, f0	  			
			} else {
	  		print "No star"
			}
     			}  # end of do loop
  		} else {   # star not withint 10 arcmin
    			print "Failed to find star"
  		}  # done with star finding commands
	} # end check of whether to go to certain star
} # end source loop

cleanup {
  offset az=0, el=0
  archive combine=1, filter=false
  close grabber
  model radio
}
