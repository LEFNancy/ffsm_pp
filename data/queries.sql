
# Queries for the upscaling and data filling of farmers in regmas.
# Database is located at http://grecof2.econ.univpm.it/phpmyadmin/


#####################################################
##               UPSCALING LEVEL                   ##
##      regional charactheristics selection        ##
#####################################################
# select 1: from main census table
SELECT 
	sum(NAZ_TOT),
# size class...
	sum(NAZ_SCL0),
	sum(NAZ_SCL1),
	sum(NAZ_SCL2),
	sum(NAZ_SCL3),
	sum(NAZ_SCL4),
	sum(NAZ_SCL5),
	sum(NAZ_SCL6),
	sum(NAZ_SCL7),
	sum(NAZ_SCL8),
# family work share class..
	sum(NAZ_WCL1),
	sum(NAZ_WCL2),
	sum(NAZ_WCL3),
	sum(NAZ_WCL4),
	sum(NAZ_WCL5),
	sum(UAA_TOT),
	sum(UAA_IRR),
	sum(LAND_AR),
	sum(LAND_TWI),
	sum(LAND_QWI),
	sum(LAND_OLO),
	sum(LAND_FRU),
	sum(LAND_GR),
# farm types...
	sum(NAZ_ARAB) + sum(NAZ_HORT) as naz_arable,
	sum(NAZ_WINE),
	sum(NAZ_OLIV),
	sum(NAZ_FRU),
	sum(NAZ_MILK),
	sum(NAZ_LIV) + sum(NAZ_GRAN) as naz_livestock ,
	sum(NAZ_MCRO) + sum(NAZ_MLIV) + sum(NAZ_MCDL) as naz_mixed,
	sum(UAA_ARAB) + sum(UAA_HORT) as uaa_arable,
	sum(UAA_WINE),
	sum(UAA_OLIV),
	sum(UAA_FRU),
	sum(UAA_MILK),
	sum(UAA_LIV) + sum(UAA_GRAN) as uaa_livestock,
	sum(UAA_MCRO) + sum(UAA_MLIV) + sum(UAA_MCDL) as uaa_mixed,
	sum(N_OVINI),
	sum(N_CAPRI),
	sum(N_AVIC)
FROM
	censagr2000_main, selmarche
WHERE
	censagr2000_main.com_istat = selmarche.com_istat

# select 2: cattle and dairy animals:
# (NB: cattle includes both beef_cattle and suckler_cows)
SELECT
   sum(`censagr2000_dairy`.`dty2clt`
	+`censagr2000_dairy`.`dty3clt`
	+`censagr2000_dairy`.`dty5clt`
	+`censagr2000_dairy`.`dty7clt`
	+`censagr2000_dairy`.`dty8clt`
	+`censagr2000_dairy`.`dty11clt`
	+`censagr2000_dairy`.`dty13clt`
	+`censagr2000_dairy`.`dty14clt`) as `cattle`,
   sum(`censagr2000_dairy`.`dty10clt`) as `dairy_cows`
FROM
	censagr2000_dairy, selmarche
WHERE
	censagr2000_dairy.com_istat = selmarche.com_istat

# select 3: sows
SELECT
   sum(`censagr2000_pigs`.`pty10clt`) as `pigs`
FROM
	censagr2000_pigs, selmarche
WHERE
	censagr2000_pigs.com_istat = selmarche.com_istat


#####################################################
##               UPSCALING LEVEL                   ##
##            farm-level selection                 ##
#####################################################
# farms data...
SELECT
	aziende.cod_az,
	aziende.ote,
	aziende.ore_unp,
	aziende.ore_paid,
	aziende.sau,
	aziende.sau_irgata
FROM `aziende`, `selmarche`
WHERE `selmarche`.`com_istat` = `aziende`.`com_istat`
# order by `colture`.`cod_az`, !!!

# land data...
SELECT
	`colture`.`cod_az`,
	sum(`colture`.`LAND_AR`),
	sum(`colture`.`LAND_GR`),
	sum(`colture`.`LAND_TWI`),
	sum(`colture`.`LAND_QWI`),
	sum(`colture`.`LAND_OLO`),
	sum(`colture`.`LAND_FRU`)
FROM
	`colture`, `selmarche`
WHERE
	`colture`.`com_istat` = `selmarche`.`com_istat`
GROUP BY
	`colture`.`cod_az`
# order by `colture`.`cod_az`, !!!

# animal data..
SELECT
	`allevamenti`.`cod_az`,
	sum(`allevamenti`.`UBA`),
	sum(`allevamenti`.`NCAT_EST`),
	sum(`allevamenti`.`NVLA_EST`),
	sum(`allevamenti`.`NPIG_EST`),
	sum(`allevamenti`.`NOV_EST`),
	sum(`allevamenti`.`NGOA_EST`),
	sum(`allevamenti`.`NAV_EST`)
FROM
	`allevamenti`, `selmarche`
WHERE
	`allevamenti`.`com_istat` = `selmarche`.`com_istat`
GROUP BY
	`allevamenti`.`cod_az`
# order by `colture`.`cod_az`, !!!


#####################################################
##            MODEL DATA FEEDING                   ##
##           farm-level selection                  ##
#####################################################

# Data from the table "aziende"...
SELECT
	aziende.cod_az,
	aziende.ote,
	aziende.ore_unp,
	aziende.sup_tot,
	aziende.sau,
	aziende.sau_aff,
	aziende.sau_irgabi,
	aziende.sau_irgata,
	aziende.cap_fond_p,
	aziende.cap_fond_a,
	aziende.cap_fond_t,
	aziende.inv_fond,
	aziende.cap_eser,
	aziende.cap_macc,
	aziende.cap_best,
	aziende.new_invest,
	aziende.deb_lungo,
	aziende.deb_medio,
	aziende.deb_cap_f,
	aziende.deb_cap_e
FROM `aziende`, `upscregmas`
WHERE `upscregmas`.`cod_az` = `aziende`.`cod_az`


# Data from the colture table...
SELECT
	`colture`.`cod_az`,
	`colture`.`class_co`,
	sum(`colture`.`sup_pri`),
	sum(`colture`.`sup_irgata`)
FROM
	`colture`, `upscregmas`
WHERE
	`colture`.`cod_az` = `upscregmas`.`cod_az`
GROUP BY
	`colture`.`cod_az`,`colture`.`class_co`
# (then used pivot_table to reorder the data)


# Data from the allevamenti table...
SELECT
	`allevamenti`.`cod_az`,
	sum(`allevamenti`.`UBA`),
	sum(`allevamenti`.`NOV_EST`),
	sum(`allevamenti`.`NAV_EST`),
	sum(`allevamenti`.`NVLA_EST`),
	sum(`allevamenti`.`NPIG_EST`),
	sum(`allevamenti`.`NCAT_EST`),
	sum(`allevamenti`.`NGOA_EST`)
FROM
	`allevamenti`, `upscregmas`
WHERE
	`allevamenti`.`cod_az` = `upscregmas`.`cod_az`
GROUP BY
	`allevamenti`.`cod_az`

# Upscaling coefficients...
SELECT *
FROM `upscregmas`



