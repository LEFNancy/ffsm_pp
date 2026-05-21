v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/nuts_lev2_france_metro_eurostat2010_projid3035.shp layer=nuts_lev2_france_metro_eurostat2010_projid3035 output=nuts_lev2_v
v.to.rast input=nuts_lev2_v@PERMANENT type=area output=nuts_lev2_r use=attr column=nutsid_int labelcolumn=NUTS_ID
v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/clc06_313_fr.shp layer=clc06_313_fr output=clc06_313_v
v.to.rast input=clc06_313_v@PERMANENT output=clc06_313_r use=attr column=Shape_Area
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4108708.651162,2321886.774919
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4090739.838090,2306878.362304
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4108881.968413,2315565.409639
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4108743.114510,2312927.185473
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4108673.687558,2310774.949969
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4113186.439420,2310219.534356
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4111936.754289,2310288.961307
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4112631.023806,2310080.680452
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4112700.450758,2315148.847928
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=4111103.630868,2315495.982687
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=3691879.299909,2501682.271533
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=3691194.817646,2513416.253191
r.what --v -f -n input=clc06_313_r@PERMANENT east_north=3682100.981861,2513709.602732
g.remove rast=clc06_313_r@PERMANENT
g.region -p res=1000
v.to.rast input=clc06_313_v@PERMANENT type=area output=clc06_313_v1000 use=val
g.region -p res=8000
r.resamp.stats clc06_313_v1000 out=clc06_313_area_km method=sum
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3675256.159227,2501535.596762
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3670171.433842,2501926.729484
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3691194.817646,2493908.508684
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3692954.914895,2503002.344469
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3690412.552202,2516007.507474
r.what --v -f -n input=clc06_313_area_km@PERMANENT east_north=3683274.380027,2517963.171084
g.region -p res=1
v.to.rast input=clc06_313_v@PERMANENT type=area output=clc06_313_r1m use=val
g.region -p res=10
v.to.rast input=clc06_313_v@PERMANENT type=area output=clc06_313_r10m use=val value=10
g.region -p res=20
v.to.rast input=clc06_313_v@PERMANENT type=area output=clc06_313_r20m use=val value=400
v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/clc06_311_fr.shp layer=clc06_311_fr output=clc06_311_v
v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/clc06_312_fr.shp layer=clc06_312_fr output=clc06_312_v
g.region -p
v.to.rast input=clc06_311_v@PERMANENT type=area output=clc06_311_r20m@PERMANENT use=val value=400
v.to.rast input=clc06_312_v@PERMANENT type=area output=clc06_312_r20m@PERMANENT use=val value=400
g.region -p res=8000
r.resamp.stats clc06_311_r20m out=clc06_311_8km_sqm method=sum
r.resamp.stats clc06_312_r20m out=clc06_312_8km_sqm method=sum
r.resamp.stats clc06_313_r20m out=clc06_313_8km_sqm method=sum
r.what --v -f -n input=clc06_311_8km_sqm@PERMANENT east_north=3787515.479216,2556663.474345
r.what --v -f -n input=clc06_311_8km_sqm@PERMANENT east_north=3756393.689948,2556542.847255
r.what --v -f -n input=clc06_311_8km_sqm@PERMANENT east_north=3685103.079648,2483563.457692
r.what --v -f -n input=clc06_311_8km_sqm@PERMANENT east_north=3813090.725634,2347033.232371
r.what --v -f -n input=clc06_312_8km_sqm@PERMANENT east_north=3426584.531281,2372537.642380
r.what --v -f -n input=clc06_312_8km_sqm@PERMANENT east_north=3420898.401108,2356862.364607
r.what --v -f -n input=clc06_312_8km_sqm@PERMANENT east_north=3428889.719189,2354249.818311
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3626831.682195,2477654.675093
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3871632.675445,2470791.095843
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3846466.218195,2331231.651094
r.out.ascii input=nuts_lev2_r@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/nuts2.grd null=-9999
g.remove rast=clc06_313_area_km@PERMANENT,nuts_lev2_r@PERMANENT,
g.remove rast=clc06_313_v1000@PERMANENT
g.remove vect=nuts_lev2_v@PERMANENT
v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/nuts_lev1_fr_metro_eurostat2010.shp layer=nuts_lev1_fr_metro_eurostat2010 output=nuts_lev1
v.in.ogr dsn=/home/lobianco/Documents/DatiGeografici/FR/nuts_lev2_france_metro_eurostat2010_projid3035.shp layer=nuts_lev2_france_metro_eurostat2010_projid3035 output=nuts_lev2_v
g.rename vect=nuts_lev1@PERMANENT,nuts_lev1_v
v.to.rast input=nuts_lev2_v@PERMANENT type=area output=nuts_lev2_r column=nutsid_i_1 labelcolumn=NUTS_ID
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3633695.261445,2884893.710592
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3798421.163445,2871166.552092
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=4047797.876194,2873454.411842
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=4031782.857944,2704152.790342
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=4130160.827194,2793379.320592
v.to.rast input=nuts_lev1_v@PERMANENT type=area output=nuts_lev1_r@PERMANENT use=attr column=nutsid_int labelcolumn=NUTS_ID
r.out.ascii input=nuts_lev1_r@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/nut2_l2.grd null=-9999
r.out.ascii input=nuts_lev1_r@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/nut2_l1.grd null=-9999
r.out.ascii input=nuts_lev2_r@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/nut_l2.grd null=-9999
r.out.ascii input=clc06_311_8km_sqm@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/clc_311.grd null=-9999
r.out.ascii input=clc06_312_8km_sqm@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/clc_312.grd null=-9999
r.out.ascii input=clc06_313_8km_sqm@PERMANENT output=/home/lobianco/Dropbox/pythia/code/pythia_clean/data/gis/france/clc_313.grd null=-9999
exit;
g.remove rast=clc06_311_8km_sqm@PERMANENT,clc06_311_r20m@PERMANENT,clc06_312_8km_sqm@PERMANENT,clc06_312_r20m@PERMANENT,clc06_313_8km_sqm@PERMANENT,clc06_313_r20m@PERMANENT
g.remove vect=clc06_311_v@PERMANENT,clc06_312_v@PERMANENT,clc06_313_v@PERMANENT,nuts_lev1_v@PERMANENT,nuts_lev2_v@PERMANENT
r.what --v -f -n input=nuts_lev2_r@PERMANENT east_north=3909478.424015,2741065.666041
exit;
exit;
