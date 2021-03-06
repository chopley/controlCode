(Count npoints, Time lststart, Time lststop) 
# number of points to record on each star, start time, stop time.


# allotting three minutes for each star, that's ~3 reps plus slew time.

group Source {
	Source name,
	Time lst}

listof Source sources = {
{  AlpAql, 0.00}, #  260.06  27.36  0.77
{  EpsPeg, 0.05}, #  237.18  49.10  2.39
{  BetPeg, 0.10}, #  237.01  75.20  2.42
{  AlpPeg, 0.15}, #  213.01  64.99  2.49
{  AlpPsA, 0.20}, #  194.57  21.79  1.16
{  BetCet, 0.25}, #  167.46  34.06  2.04
{  AlpAnd, 0.30}, #  166.89  81.90  2.06
{  AlpAri, 0.35}, #  107.38  59.57  2.00
{  BetAnd, 0.40}, #   90.38  75.90  2.06
{  AlpTau, 0.45}, #   88.72  26.47  0.85
{  BetTau, 0.50}, #   70.65  23.01  1.65
{  BetPer, 0.55}, #   68.86  53.66  2.12
{ Gam1And, 0.60}, #   67.66  65.71  2.26
{  AlpAur, 0.65}, #   53.99  32.49  0.08
{  AlpPer, 0.70}, #   53.82  51.58  1.79
{  BetAur, 0.75}, #   51.48  25.23  1.90
{  GamCas, 0.80}, #   16.24  64.64  2.47
{  AlpCas, 0.85}, #   15.93  69.32  2.23
{  BetCas, 0.90}, #    3.11  67.80  2.27
{  AlpUMi, 0.95}, #    0.57  37.58  2.02
{  BetUMi, 1.00}, #  -11.76  24.75  2.08
{  AlpCep, 1.05}, #  -30.92  54.53  2.44
{  GamDra, 1.10}, #  -44.64  27.62  2.23
{  AlpLyr, 1.15}, #  -61.09  28.48  0.03
{  AlpCyg, 1.20}, #  -61.32  52.33  1.25
{  GamCyg, 1.25}, #  -68.15  48.02  2.20
{  EpsCyg, 1.30}, #  -79.49  50.86  2.46
{  BetPeg, 1.35}, #  262.82  59.92  2.42
{  EpsPeg, 1.40}, #  255.29  34.29  2.39
{  AlpAnd, 1.45}, #  247.74  72.89  2.06
{  AlpPeg, 1.50}, #  243.31  52.78  2.49
{  BetCet, 1.55}, #  190.79  34.31  2.04
{  AlpAri, 1.60}, #  140.53  73.22  2.00
{  DelOri, 1.65}, #  107.46  21.25  2.23
{  GamOri, 1.70}, #  103.04  26.81  1.64
{  AlpTau, 1.75}, #  102.04  42.54  0.85
{  AlpOri, 1.80}, #   97.12  21.55  0.50
{  BetTau, 1.85}, #   80.50  38.65  1.65
{  BetPer, 1.90}, #   70.92  68.89  2.12
{  AlpAur, 1.95}, #   58.92  46.00  0.08
{  BetAur, 2.00}, #   57.92  38.45  1.90
{ Gam1And, 2.05}, #   53.81  80.18  2.26
{  AlpPer, 2.10}, #   49.98  64.44  1.79
{  AlpUMi, 2.15}, #    0.28  37.70  2.02
{  BetUMi, 2.20}, #   -6.46  22.17  2.08
{  GamCas, 2.25}, #   -7.30  65.97  2.47
{  AlpCas, 2.30}, #  -15.93  69.32  2.23
{  BetCas, 2.35}, #  -21.96  64.99  2.27
{  AlpCep, 2.40}, #  -35.00  45.62  2.44
{  AlpCyg, 2.45}, #  -57.44  38.38  1.25
{  GamCyg, 2.50}, #  -61.79  33.35  2.20
{  EpsCyg, 2.55}, #  -71.07  35.23  2.46
{  AlpAnd, 2.60}, #  266.48  58.28  2.06
{  AlpPeg, 2.65}, #  259.56  38.59  2.49
{  BetCet, 2.70}, #  210.80  28.96  2.04
{  AlpAri, 2.75}, #  206.61  75.11  2.00
{  BetOri, 2.80}, #  132.21  31.53  0.12
{  KapOri, 2.85}, #  125.99  25.24  2.06
{  DelOri, 2.90}, #  122.18  34.84  2.23
{  EpsOri, 2.95}, #  121.98  33.47  1.70
{  AlpTau, 3.00}, #  119.66  56.54  0.85
{  GamOri, 3.05}, #  117.91  40.83  1.64
{  AlpOri, 3.10}, #  110.44  36.07  0.50
{  GamGem, 3.15}, #   94.08  33.23  1.93
{  BetTau, 3.20}, #   90.65  53.56  1.65
{  BetGem, 3.25}, #   73.28  25.94  1.14
{  BetAur, 3.30}, #   61.72  51.42  1.90
{  AlpAur, 3.35}, #   60.36  58.96  0.08
{  BetPer, 3.40}, #   55.19  82.61  2.12
{  AlpPer, 3.45}, #   30.27  74.47  1.79
{  AlpUMi, 3.50}, #   -0.02  37.74  2.02
{  BetUMi, 3.55}, #   -1.08  21.18  2.08
{  GamCas, 3.60}, #  -25.56  61.58  2.47
{  AlpCep, 3.65}, #  -34.53  37.02  2.44
{  BetCas, 3.70}, #  -34.97  57.67  2.27
{  AlpCas, 3.75}, #  -35.33  62.58  2.23
{ Gam1And, 3.80}, #  -46.70  81.85  2.26
{  AlpCyg, 3.85}, #  -51.60  26.16  1.25
{  GamCyg, 3.90}, #  -54.49  20.63  2.20
{  EpsCyg, 3.95}, #  -62.90  21.46  2.46
{  BetPeg, 4.00}, #  -85.39  44.97  2.42
{  BetAnd, 4.05}, #  -88.05  72.60  2.06
{  AlpAri, 4.10}, #  250.44  61.16  2.00
{  AlpTau, 4.15}, #  160.12  68.47  0.85
{  BetOri, 4.20}, #  156.88  42.03  0.12
{  KapOri, 4.25}, #  147.70  37.58  2.06
{  DelOri, 4.30}, #  146.99  47.72  2.23
{  EpsOri, 4.35}, #  146.18  46.45  1.70
{  GamOri, 4.40}, #  144.67  54.43  1.64
{  BetCMa, 4.45}, #  143.41  26.30  1.98
{  AlpCMa, 4.50}, #  137.66  24.50 -1.46
{  AlpOri, 4.55}, #  133.23  51.39  0.50
{  GamGem, 4.60}, #  111.30  50.74  1.93
{  BetTau, 4.65}, #  110.70  71.21  1.65
{  AlpCMi, 4.70}, #  109.65  31.99  0.38
{  BetGem, 4.75}, #   84.52  43.54  1.14
{  BetAur, 4.80}, #   60.61  67.29  1.90
{  AlpAur, 4.85}, #   50.13  74.05  0.08
{  BetUMa, 4.90}, #   35.78  23.23  2.37
{  AlpUMa, 4.95}, #   30.62  25.84  1.79
{  BetUMi, 5.00}, #    5.45  21.87  2.08
{  AlpUMi, 5.05}, #   -0.37  37.67  2.02
{  AlpPer, 5.10}, #  -26.98  75.11  1.79
{  AlpCep, 5.15}, #  -30.47  27.30  2.44
{  GamCas, 5.20}, #  -35.89  52.16  2.47
{  BetCas, 5.25}, #  -39.86  46.58  2.27
{  AlpCas, 5.30}, #  -43.18  50.98  2.23
{  BetPer, 5.35}, #  -66.15  78.10  2.12
{ Gam1And, 5.40}, #  -67.63  66.04  2.26
{  BetPeg, 5.45}, #  -74.09  27.33  2.42
{  BetAnd, 5.50}, #  -78.64  54.78  2.06
{  AlpAnd, 5.55}, #  -80.89  40.38  2.06
{  AlpPeg, 5.60}, #  -86.19  20.69  2.49
{  AlpAri, 5.65}, #  268.86  42.92  2.00
{  AlpTau, 5.70}, #  218.44  65.22  0.85
{  BetTau, 5.75}, #  198.53  81.19  1.65
{  BetOri, 5.80}, #  188.49  44.44  0.12
{  GamOri, 5.85}, #  186.74  59.19  1.64
{  DelOri, 5.90}, #  182.88  52.67  2.23
{  EpsOri, 5.95}, #  181.13  51.79  1.70
{  KapOri, 6.00}, #  177.03  43.29  2.06
{  AlpOri, 6.05}, #  171.92  60.18  0.50
{  BetCMa, 6.10}, #  167.43  34.09  1.98
{  EpsCMa, 6.15}, #  161.34  21.43  1.50
{  AlpCMa, 6.20}, #  160.79  34.07 -1.46
{  DelCMa, 6.25}, #  158.25  23.21  1.84
{  GamGem, 6.30}, #  143.87  65.65  1.93
{  AlpCMi, 6.35}, #  131.78  47.99  0.38
{  BetGem, 6.40}, #   99.50  62.05  1.14
{  AlpLeo, 6.45}, #   93.88  25.20  1.35
{  BetUMa, 6.50}, #   41.79  34.93  2.37
{  GamUMa, 6.55}, #   41.55  27.03  2.44
{  AlpUMa, 6.60}, #   35.33  36.02  1.79
{  EpsUMa, 6.65}, #   34.54  20.79  1.77
{  BetAur, 6.70}, #   24.34  81.16  1.90
{  BetUMi, 6.75}, #   11.61  24.65  2.08
{  AlpUMi, 6.80}, #   -0.68  37.50  2.02
{  AlpAur, 6.85}, #  -23.09  80.09  0.08
{  GamCas, 6.90}, #  -37.61  40.91  2.47
{  BetCas, 6.95}, #  -38.24  34.78  2.27
{  AlpCas, 7.00}, #  -42.60  38.23  2.23
{  AlpPer, 7.05}, #  -51.17  62.69  1.79
{ Gam1And, 7.10}, #  -65.25  48.95  2.26
{  AlpAnd, 7.15}, #  -69.67  22.45  2.06
{  BetAnd, 7.20}, #  -69.76  36.93  2.06
{  BetPer, 7.25}, #  -70.46  60.65  2.12
{  BetTau, 7.30}, #  258.40  65.18  1.65
{  AlpTau, 7.35}, #  251.23  48.86  0.85
{  GamOri, 7.40}, #  226.97  49.99  1.64
{  DelOri, 7.45}, #  219.42  45.36  2.23
{  BetOri, 7.50}, #  219.08  36.35  0.12
{  AlpOri, 7.55}, #  217.60  54.92  0.50
{  EpsOri, 7.60}, #  217.49  45.10  1.70
{  KapOri, 7.65}, #  209.01  38.75  2.06
{  GamGem, 7.70}, #  206.07  67.56  1.93
{  BetCMa, 7.75}, #  195.82  33.52  1.98
{  AlpCMa, 7.80}, #  189.70  35.73 -1.46
{  EpsCMa, 7.85}, #  184.63  23.87  1.50
{  DelCMa, 7.90}, #  182.41  26.57  1.84
{  EtaCMa, 7.95}, #  178.55  23.68  2.45
{  AlpCMi, 8.00}, #  170.00  57.85  0.38
{  BetGem, 8.05}, #  145.41  79.34  1.14
{  AlpHya, 8.10}, #  139.61  35.17  1.98
{  AlpLeo, 8.15}, #  112.02  44.42  1.35
{  BetLeo, 8.20}, #   91.25  26.35  2.14
{  GamUMa, 8.25}, #   46.98  40.90  2.44
{  EtaUMa, 8.30}, #   44.58  22.92  1.86
{  BetUMa, 8.35}, #   43.79  48.48  2.37
{  EpsUMa, 8.40}, #   41.62  33.05  1.77
{  AlpUMa, 8.45}, #   35.84  47.66  1.79
{  BetUMi, 8.50}, #   16.77  29.55  2.08
{  AlpUMi, 8.55}, #   -0.88  37.23  2.02
{  BetCas, 8.60}, #  -32.23  23.30  2.27
{  GamCas, 8.65}, #  -33.93  29.25  2.47
{  AlpCas, 8.70}, #  -37.06  25.49  2.23
{  AlpPer, 8.75}, #  -53.43  46.86  1.79
{  BetAur, 8.80}, #  -55.55  73.25  1.90
{ Gam1And, 8.85}, #  -58.20  31.51  2.26
{  AlpAur, 8.90}, #  -58.66  65.75  0.08
{  BetPer, 8.95}, #  -64.92  42.33  2.12
{  AlpAri, 9.00}, #  -77.39  23.30  2.00
{  AlpTau, 9.05}, #  269.96  28.23  0.85
{  GamOri, 9.10}, #  252.22  31.86  1.64
{  AlpOri, 9.15}, #  247.22  38.18  0.50
{  GamGem, 9.20}, #  247.13  51.92  1.93
{  BetGem, 9.25}, #  246.81  71.38  1.14
{  DelOri, 9.30}, #  245.16  28.70  2.23
{  EpsOri, 9.35}, #  243.59  28.83  1.70
{  BetOri, 9.40}, #  242.25  20.08  0.12
{  KapOri, 9.45}, #  234.72  24.67  2.06
{  BetCMa, 9.50}, #  221.83  23.32  1.98
{  AlpCMa, 9.55}, #  217.55  27.23 -1.46
{  AlpCMi, 9.60}, #  216.14  52.91  0.38
{  DelCMa, 9.65}, #  207.41  21.12  1.84
{  AlpHya, 9.70}, #  171.54  43.98  1.98
{  AlpLeo, 9.75}, #  145.42  60.99  1.35
{  BetLeo, 9.80}, #  110.14  46.90  2.14
{  AlpBoo, 9.85}, #   81.03  20.64 -0.04
{  EtaUMa, 9.90}, #   52.19  38.66  1.86
{  GamUMa, 9.95}, #   46.45  56.35  2.44
{  EpsUMa, 10.00}, #   44.48  47.49  1.77
{  BetUMa, 10.05}, #   36.14  62.28  2.37
{  AlpUMa, 10.10}, #   27.49  58.97  1.79
{  BetUMi, 10.15}, #   19.75  36.20  2.08
{  AlpUMi, 10.20}, #   -0.91  36.90  2.02
{  AlpPer, 10.25}, #  -48.17  30.52  1.79
{  BetPer, 10.30}, #  -55.79  24.08  2.12
{  AlpAur, 10.35}, #  -59.30  47.60  0.08
{  BetAur, 10.40}, #  -62.22  54.96  1.90
{  BetTau, 10.45}, #  -84.13  44.31  1.65
{  BetGem, 10.50}, #  267.57  54.43  1.14
{  GamGem, 10.55}, #  264.40  35.11  1.93
{  AlpOri, 10.60}, #  262.98  21.42  0.50
{  AlpCMi, 10.65}, #  241.41  39.76  0.38
{  AlpHya, 10.70}, #  200.98  42.05  1.98
{  AlpLeo, 10.75}, #  192.36  64.50  1.35
{  BetLeo, 10.80}, #  136.69  61.50  2.14
{  AlpVir, 10.85}, #  130.54  26.68  0.98
{  AlpBoo, 10.90}, #   93.78  37.95 -0.04
{  AlpCrB, 10.95}, #   75.12  26.17  2.23
{  EtaUMa, 11.00}, #   54.72  52.68  1.86
{  EpsUMa, 11.05}, #   40.21  59.35  1.77
{  GamUMa, 11.10}, #   34.27  67.91  2.44
{  BetUMi, 11.15}, #   19.51  42.11  2.08
{  BetUMa, 11.20}, #   12.89  69.89  2.37
{  AlpUMa, 11.25}, #    9.34  64.70  1.79
{  AlpUMi, 11.30}, #   -0.80  36.63  2.02
{  AlpAur, 11.35}, #  -54.24  33.03  0.08
{  BetAur, 11.40}, #  -58.44  39.82  1.90
{  BetTau, 11.45}, #  -73.40  27.30  1.65
{  AlpCMi, 11.50}, #  253.60  28.69  0.38
{  AlpLeo, 11.55}, #  221.45  58.98  1.35
{  AlpHya, 11.60}, #  218.52  36.08  1.98
{  BetLeo, 11.65}, #  168.04  67.17  2.14
{  AlpVir, 11.70}, #  144.88  34.76  0.98
{  AlpBoo, 11.75}, #  104.69  49.77 -0.04
{  AlpCrB, 11.80}, #   82.73  37.92  2.23
{  EtaUMa, 11.85}, #   52.57  62.38  1.86
{  GamDra, 11.90}, #   42.78  24.41  2.23
{  EpsUMa, 11.95}, #   30.02  66.35  1.77
{  BetUMi, 12.00}, #   17.55  45.94  2.08
{  GamUMa, 12.05}, #   12.00  72.80  2.44
{  AlpUMi, 12.10}, #   -0.65  36.48  2.02
{  AlpUMa, 12.15}, #   -7.34  64.91  1.79
{  BetUMa, 12.20}, #  -11.47  70.05  2.37
{  AlpAur, 12.25}, #  -49.19  23.62  0.08
{  BetAur, 12.30}, #  -54.03  29.85  1.90
{  BetGem, 12.35}, #  -83.81  42.47  1.14
{  GamGem, 12.40}, #  -86.42  23.14  1.93
{  AlpLeo, 12.45}, #  240.19  50.12  1.35
{  AlpHya, 12.50}, #  232.14  27.98  1.98
{  BetLeo, 12.55}, #  203.12  66.03  2.14
{  AlpVir, 12.60}, #  161.25  39.93  0.98
{  AlpBoo, 12.65}, #  119.43  60.32 -0.04
{  AlpCrB, 12.70}, #   90.71  49.27  2.23
{  AlpLyr, 12.75}, #   56.24  20.49  0.03
{  GamDra, 12.80}, #   47.01  32.46  2.23
{  EtaUMa, 12.85}, #   43.41  70.94  1.86
{  BetUMi, 12.90}, #   14.15  49.07  2.08
{  EpsUMa, 12.95}, #   11.38  70.50  1.77
{  AlpUMi, 13.00}, #   -0.47  36.37  2.02
{  GamUMa, 13.05}, #  -16.35  72.35  2.44
{  AlpUMa, 13.10}, #  -21.04  62.06  1.79
{  BetUMa, 13.15}, #  -29.55  65.94  2.37
{  BetAur, 13.20}, #  -48.80  20.95  1.90
{  BetGem, 13.25}, #  -76.66  31.27  1.14
{  AlpLeo, 13.30}, #  252.08  40.81  1.35
{  BetLeo, 13.35}, #  227.28  60.10  2.14
{  AlpVir, 13.40}, #  177.63  41.81  0.98
{  AlpBoo, 13.45}, #  140.78  68.17 -0.04
{  AlpCrB, 13.50}, #   99.70  59.40  2.23
{  AlpOph, 13.55}, #   95.31  28.02  2.08
{  AlpLyr, 13.60}, #   61.49  29.20  0.03
{  GamDra, 13.65}, #   49.73  40.08  2.23
{  AlpCep, 13.70}, #   25.15  20.46  2.44
{  EtaUMa, 13.75}, #   21.16  76.58  1.86
{  BetUMi, 13.80}, #    9.88  51.21  2.08
{  AlpUMi, 13.85}, #   -0.29  36.30  2.02
{  EpsUMa, 13.90}, #  -10.14  70.61  1.77
{  AlpUMa, 13.95}, #  -29.31  57.68  1.79
{  GamUMa, 14.00}, #  -34.35  67.87  2.44
{  BetUMa, 14.05}, #  -38.55  60.18  2.37
{  BetGem, 14.10}, #  -70.40  21.51  1.14
{  AlpLeo, 14.15}, #  261.46  30.90  1.35
{  BetLeo, 14.20}, #  243.53  51.69  2.14
{  AlpVir, 14.25}, #  194.24  40.75  0.98
{  AlpBoo, 14.30}, #  174.87  72.12 -0.04
{  DelSco, 14.35}, #  151.64  24.86  2.32
{  EtaOph, 14.40}, #  132.32  22.16  2.43
{  AlpCrB, 14.45}, #  113.43  69.17  2.23
{  AlpOph, 14.50}, #  104.12  38.05  2.08
{  AlpLyr, 14.55}, #   66.17  38.34  0.03
{  GamCyg, 14.60}, #   54.54  20.72  2.20
{  GamDra, 14.65}, #   51.14  47.94  2.23
{  AlpCyg, 14.70}, #   48.00  20.39  1.25
{  AlpCep, 14.75}, #   29.03  25.11  2.44
{  BetUMi, 14.80}, #    4.66  52.50  2.08
{  AlpUMi, 14.85}, #   -0.09  36.27  2.02
{  EtaUMa, 14.90}, #  -15.83  77.09  1.86
{  EpsUMa, 14.95}, #  -27.69  67.23  1.77
{  AlpUMa, 15.00}, #  -34.04  52.30  1.79
{  BetUMa, 15.05}, #  -42.72  53.50  2.37
{  GamUMa, 15.10}, #  -43.46  61.40  2.44
{  BetLeo, 15.15}, #  256.82  40.42  2.14
{  AlpBoo, 15.20}, #  216.85  68.70 -0.04
{  AlpVir, 15.25}, #  212.06  36.02  0.98
{  DelSco, 15.30}, #  166.42  29.16  2.32
{  AlpSco, 15.35}, #  160.34  23.81  0.96
{  AlpCrB, 15.40}, #  150.30  78.37  2.23
{  EtaOph, 15.45}, #  145.83  30.02  2.43
{  AlpOph, 15.50}, #  117.23  49.26  2.08
{  AlpAql, 15.55}, #   94.66  20.91  0.77
{  AlpLyr, 15.60}, #   71.01  49.50  0.03
{  EpsCyg, 15.65}, #   64.33  23.70  2.46
{  GamCyg, 15.70}, #   60.49  30.83  2.20
{  AlpCyg, 15.75}, #   53.51  29.67  1.25
{  GamDra, 15.80}, #   50.24  57.25  2.23
{  AlpCep, 15.85}, #   32.56  31.26  2.44
{  AlpUMi, 15.90}, #    0.15  36.27  2.02
{  BetUMi, 15.95}, #   -2.06  52.78  2.08
{  AlpUMa, 16.00}, #  -36.23  45.36  1.79
{  EpsUMa, 16.05}, #  -39.17  60.51  1.77
{  EtaUMa, 16.10}, #  -43.65  70.81  1.86
{  BetUMa, 16.15}, #  -43.87  45.25  2.37
{  GamUMa, 16.20}, #  -47.45  52.80  2.44
{  BetLeo, 16.25}, #  267.97  27.37  2.14
{  AlpBoo, 16.30}, #  243.85  58.44 -0.04
{  AlpVir, 16.35}, #  228.29  27.49  0.98
{  AlpCrB, 16.40}, #  222.55  76.64  2.23
{  DelSco, 16.45}, #  183.92  30.28  2.32
{  AlpSco, 16.50}, #  176.40  26.48  0.96
{  EtaOph, 16.55}, #  163.53  35.68  2.43
{  AlpOph, 16.60}, #  138.60  59.71  2.08
{  AlpAql, 16.65}, #  105.85  33.86  0.77
{  AlpLyr, 16.70}, #   75.35  62.12  0.03
{  EpsCyg, 16.75}, #   71.45  35.91  2.46
{  GamCyg, 16.80}, #   66.03  42.60  2.20
{  AlpCyg, 16.85}, #   58.27  40.60  1.25
{  GamDra, 16.90}, #   43.00  66.94  2.23
{  AlpCep, 16.95}, #   34.84  38.60  2.44
{  AlpUMi, 17.00}, #    0.40  36.34  2.02
{  BetUMi, 17.05}, #   -9.07  51.48  2.08
{  AlpUMa, 17.10}, #  -35.72  37.58  1.79
{  BetUMa, 17.15}, #  -42.22  36.22  2.37
{  EpsUMa, 17.20}, #  -43.95  51.68  1.77
{  GamUMa, 17.25}, #  -47.40  43.05  2.44
{  EtaUMa, 17.30}, #  -53.28  60.80  1.86
{  AlpBoo, 17.35}, #  259.22  45.93 -0.04
{  AlpCrB, 17.40}, #  253.06  65.31  2.23
{  DelSco, 17.45}, #  201.01  27.42  2.32
{  AlpSco, 17.50}, #  192.79  25.42  0.96
{  EtaOph, 17.55}, #  183.21  37.22  2.43
{  AlpOph, 17.60}, #  171.82  65.36  2.08
{  SigSgr, 17.65}, #  157.10  22.93  2.02
{  AlpAql, 17.70}, #  120.15  45.99  0.77
{  EpsPeg, 17.75}, #   96.73  25.20  2.39
{  EpsCyg, 17.80}, #   78.29  48.62  2.46
{  AlpLyr, 17.85}, #   77.41  74.94  0.03
{  GamCyg, 17.90}, #   70.41  54.84  2.20
{  AlpCyg, 17.95}, #   61.27  52.00  1.25
{  AlpCep, 18.00}, #   34.91  46.17  2.44
{  BetCas, 18.05}, #   33.77  25.54  2.27
{  AlpCas, 18.10}, #   33.65  20.63  2.23
{  GamCas, 18.15}, #   28.72  21.50  2.47
{  GamDra, 18.20}, #   20.72  74.20  2.23
{  AlpUMi, 18.25}, #    0.62  36.45  2.02
{  BetUMi, 18.30}, #  -14.62  48.74  2.08
{  AlpUMa, 18.35}, #  -33.06  30.11  1.79
{  BetUMa, 18.40}, #  -38.53  27.66  2.37
{  EpsUMa, 18.45}, #  -44.20  42.47  1.77
{  GamUMa, 18.50}, #  -44.67  33.54  2.44
{  EtaUMa, 18.55}, #  -54.62  50.09  1.86
{  AlpCrB, 18.60}, #  268.29  50.54  2.23
{  DelSco, 18.65}, #  218.10  20.00  2.32
{  AlpOph, 18.70}, #  213.18  61.99  2.08
{  EtaOph, 18.75}, #  204.93  33.55  2.43
{  SigSgr, 18.80}, #  175.17  26.54  2.02
{  AlpAql, 18.85}, #  144.26  57.16  0.77
{  EpsPeg, 18.90}, #  110.32  39.74  2.39
{  AlpPeg, 18.95}, #   91.01  27.17  2.49
{  EpsCyg, 19.00}, #   86.72  63.45  2.46
{  BetPeg, 19.05}, #   78.31  34.00  2.42
{  GamCyg, 19.10}, #   72.85  69.08  2.20
{  AlpAnd, 19.15}, #   69.46  22.12  2.06
{  AlpCyg, 19.20}, #   60.65  65.16  1.25
{  AlpCas, 19.25}, #   39.33  29.56  2.23
{  BetCas, 19.30}, #   38.10  34.37  2.27
{  GamCas, 19.35}, #   33.97  29.32  2.47
{  AlpCep, 19.40}, #   31.02  54.41  2.44
{  AlpUMi, 19.45}, #    0.80  36.64  2.02
{  BetUMi, 19.50}, #  -18.55  44.42  2.08
{  GamDra, 19.55}, #  -22.61  73.92  2.23
{  AlpUMa, 19.60}, #  -28.11  22.46  1.79
{  GamUMa, 19.65}, #  -39.39  23.48  2.44
{  EpsUMa, 19.70}, #  -41.30  32.26  1.77
{  EtaUMa, 19.75}, #  -51.98  38.04  1.86
{  AlpBoo, 19.80}, #  -88.46  31.03 -0.04
{  AlpOph, 19.85}, #  240.16  50.99  2.08
{  EtaOph, 19.90}, #  223.28  25.12  2.43
{  SigSgr, 19.95}, #  193.82  25.36  2.02
{  AlpAql, 20.00}, #  180.11  61.87  0.77
{  EpsPeg, 20.05}, #  129.51  52.75  2.39
{  AlpPeg, 20.10}, #  103.68  42.00  2.49
{  EpsCyg, 20.15}, #  100.99  78.37  2.46
{  BetPeg, 20.20}, #   88.08  48.85  2.42
{  AlpAnd, 20.25}, #   78.48  36.49  2.06
{  BetAnd, 20.30}, #   64.73  27.83  2.06
{ Gam1And, 20.35}, #   52.67  21.84  2.26
{  AlpCyg, 20.40}, #   44.88  77.43  1.25
{  AlpCas, 20.45}, #   42.89  39.45  2.23
{  BetCas, 20.50}, #   39.94  43.84  2.27
{  GamCas, 20.55}, #   37.15  38.07  2.47
{  AlpCep, 20.60}, #   20.81  61.09  2.44
{  AlpUMi, 20.65}, #    0.91  36.87  2.02
{  BetUMi, 20.70}, #  -19.97  39.43  2.08
{  EpsUMa, 20.75}, #  -36.08  22.87  1.77
{  GamDra, 20.80}, #  -45.21  65.17  2.23
{  EtaUMa, 20.85}, #  -46.83  26.65  1.86
{  AlpLyr, 20.90}, #  -77.38  75.31  0.03
{  AlpCrB, 20.95}, #  -81.22  35.62  2.23
{  AlpOph, 21.00}, #  255.71  38.22  2.08
{  AlpAql, 21.05}, #  213.47  57.80  0.77
{  SigSgr, 21.10}, #  209.65  20.24  2.02
{  EpsPeg, 21.15}, #  157.01  61.10  2.39
{  AlpPeg, 21.20}, #  119.90  54.80  2.49
{  BetPeg, 21.25}, #   99.93  62.58  2.42
{  AlpAnd, 21.30}, #   87.29  50.15  2.06
{  AlpAri, 21.35}, #   77.88  24.02  2.00
{  BetAnd, 21.40}, #   71.67  40.61  2.06
{ Gam1And, 21.45}, #   59.06  33.25  2.26
{  BetPer, 21.50}, #   54.43  21.90  2.12
{  AlpPer, 21.55}, #   44.53  23.93  1.79
{  AlpCas, 21.60}, #   43.51  48.92  2.23
{  BetCas, 21.65}, #   38.35  52.60  2.27
{  GamCas, 21.70}, #   37.58  46.47  2.47
{  AlpCep, 21.75}, #    4.92  64.25  2.44
{  AlpUMi, 21.80}, #    0.92  37.09  2.02
{  BetUMi, 21.85}, #  -19.39  34.76  2.08
{  AlpCyg, 21.90}, #  -21.39  81.02  1.25
{  GamDra, 21.95}, #  -50.84  54.82  2.23
{  GamCyg, 22.00}, #  -63.28  81.94  2.20
{  AlpCrB, 22.05}, #  -72.53  22.22  2.23
{  AlpLyr, 22.10}, #  -75.29  61.90  0.03
{  AlpOph, 22.15}, #  267.39  24.61  2.08
{  EpsCyg, 22.20}, #  265.97  72.91  2.46
{  AlpAql, 22.25}, #  236.81  47.96  0.77
{  EpsPeg, 22.30}, #  193.24  62.30  2.39
{  AlpPsA, 22.35}, #  168.57  22.41  1.16
{  AlpPeg, 22.40}, #  146.72  64.93  2.49
{  BetCet, 22.45}, #  139.43  24.04  2.04
{  BetPeg, 22.50}, #  123.82  75.44  2.42
{  AlpAnd, 22.55}, #   98.87  63.89  2.06
{  AlpAri, 22.60}, #   87.24  37.66  2.00
{  BetAnd, 22.65}, #   78.21  53.91  2.06
{ Gam1And, 22.70}, #   64.10  45.38  2.26
{  BetPer, 22.75}, #   60.98  33.55  2.12
{  AlpPer, 22.80}, #   49.75  34.04  1.79
{  AlpCas, 22.85}, #   39.94  58.17  2.23
{  GamCas, 22.90}, #   34.39  54.64  2.47
{  BetCas, 22.95}, #   31.53  60.60  2.27
{  AlpUMi, 23.00}, #    0.84  37.30  2.02
{  AlpCep, 23.05}, #  -12.90  63.26  2.44
{  BetUMi, 23.10}, #  -17.35  30.39  2.08
{  GamDra, 23.15}, #  -50.65  44.10  2.23
{  AlpCyg, 23.20}, #  -56.36  71.63  1.25
{  AlpLyr, 23.25}, #  -70.70  48.72  0.03
{  GamCyg, 23.30}, #  -72.86  68.98  2.20
{  AlpAql, 23.35}, #  253.09  34.93  0.77
{  EpsPeg, 23.40}, #  225.01  55.17  2.39
{  BetPeg, 23.45}, #  203.34  80.38  2.42
{  AlpPeg, 23.50}, #  190.47  67.91  2.49
{  AlpPsA, 23.55}, #  185.52  23.15  1.16
{  BetCet, 23.60}, #  156.79  31.67  2.04
{  AlpAnd, 23.65}, #  125.17  77.32  2.06
{  AlpAri, 23.70}, #   98.90  51.99  2.00
{  BetAnd, 23.75}, #   85.39  68.13  2.06
{ Gam1And, 23.80}, #   67.42  58.51  2.26
{  BetPer, 23.85}, #   66.51  46.45  2.12
{  AlpPer, 23.90}, #   53.18  45.32  1.79
{  AlpAur, 23.95} #   50.79  26.32  0.08
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
setDefaultFrameGrabberChannel chan1
configureFrameGrabber chan=chan1
setOpticalCameraRotation angle=180
configureFrameGrabber flatfield=image
setOpticalCameraFov fov=11.5, chan=chan1
addSearchBox 11, 12, 495, 465, true, chan=chan1


# let's first take a flatfield
#slew az=180, el=80
#until $acquired(source)
#takeFlatfield chan=chan1
configureFrameGrabber flatfield=image, chan=chan1
configureFrameGrabber combine=10


# wait for lst start
until $after($lststart,lst)


foreach(Source source) $sources {

	if $time(lst)>$lststop {
		log "lststop reached"
		break
  	}

	if($time(lst)>$source.lst) { 
		log "too late for source ", $source.name
 	} else if($elevation($source.name)>20) {
	 
		#go to source
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


  		if($imstat(snr) > 5 & $peak(xabs) < 00:11:00 & $peak(yabs) < 00:11:00) {
     			print "Found star at ",$peak(x),",",$peak(y)," with snr ",$imstat(snr)
     			center # offset to star
     			until $acquired(source)
     			until $elapsed>5s # always wait for the telescope to settle after center

     			# Now record a few points to sample the seeing disk
     			do Count i=1,$npoints,1 {
				grabFrame chan=chan1
				until $acquired(grab)
				until $elapsed>3s

			if($imstat(snr) > 5 & $peak(xabs) < 00:05:00 & $peak(yabs) < 00:05:00) {
	  			print "Star offset ",$peak(x),",",$peak(y),", snr ",$imstat(snr)
	  			center 
				until $acquired(source)
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
