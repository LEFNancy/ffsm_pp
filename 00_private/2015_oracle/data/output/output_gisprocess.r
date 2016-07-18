library(raster)
# obj:
# - Share of broadleaved/coniferous: map now/2100 by pixel
# - Vol/ha of forest: map now/2100 by pixel
# - map share broadleaved forest by pixel 2100 managed/unmanaged

mapsdir <- 'maps/asciiGrids/'
area_prefix <- 'forArea'
vol_prefix  <- 'vol'
scenario <- 'arpege_a1b'

years <- c('2015','2100')
spGroup <- list('broadleaved' = list('broadL_highF', 'broadL_mixedF','broadL_copp'), 'coniferous'= list('con'))
spGroup_inv <- list('broadL_highF' = 'broadleaved', 'broadL_mixedF' = 'broadleaved','broadL_copp' = 'broadleaved', 'con' = 'broadleaved')

prefix_area = paste0(mapsdir,area_prefix)
prefix_vol  = paste0(mapsdir,vol_prefix)
forArea_pc_maps <- list()
volHa_maps      <- list()

for (year in years) {
  forArea_broadL_highF_filename  <- paste(prefix_area,'broadL_highF',year,scenario, sep='_')
  forArea_broadL_mixedF_filename <- paste(prefix_area,'broadL_mixedF',year,scenario, sep='_')
  forArea_broadL_copp_filename   <- paste(prefix_area,'broadL_copp',year,scenario, sep='_')
  forArea_con_highF_filename     <- paste(prefix_area,'con_highF',year,scenario, sep='_')
  vol_broadL_highF_filename      <- paste(prefix_vol,'broadL_highF',year,scenario, sep='_')
  vol_broadL_mixedF_filename     <- paste(prefix_vol,'broadL_mixedF',year,scenario, sep='_')
  vol_broadL_copp_filename       <- paste(prefix_vol,'broadL_copp',year,scenario, sep='_')
  vol_con_highF_filename         <- paste(prefix_vol,'con_highF',year,scenario, sep='_')  
  
  forArea_broadL_highF  = raster(forArea_broadL_highF_filename)
  forArea_broadL_mixedF = raster(forArea_broadL_mixedF_filename)
  forArea_broadL_copp   = raster(forArea_broadL_copp_filename)
  forArea_con_highF     = raster(forArea_con_highF_filename)
  vol_broadL_highF      = raster(vol_broadL_highF_filename)
  vol_broadL_mixedF     = raster(vol_broadL_mixedF_filename)
  vol_broadL_copp       = raster(vol_broadL_copp_filename)
  vol_con_highF         = raster(vol_con_highF_filename)
  
  forArea_broadL        = forArea_broadL_highF + forArea_broadL_mixedF + forArea_broadL_copp
  forArea_tot           = forArea_con_highF + forArea_broadL
  vol_tot               = vol_broadL_highF + vol_broadL_mixedF + vol_broadL_copp + vol_con_highF
  forArea_con_pc        = 100 * forArea_con_highF / forArea_tot
  volHa                 = vol_tot*1000000/(forArea_tot/10000) # Vol in Mm^3, Area in m^2 -> vHa in m^3/ha
  forArea_pc_maps[[length(forArea_pc_maps)+1]] <- forArea_con_pc
  volHa_maps[[length(volHa_maps)+1]] <- volHa
}


scenarios <- c('arpege_a1b_nomgm', 'arpege_a1b_noexp','arpege_a1b_fullexp')
year <- '2100'

for (scenario in scenarios) {
  forArea_broadL_highF_filename  <- paste(prefix_area,'broadL_highF',year,scenario, sep='_')
  forArea_broadL_mixedF_filename <- paste(prefix_area,'broadL_mixedF',year,scenario, sep='_')
  forArea_broadL_copp_filename   <- paste(prefix_area,'broadL_copp',year,scenario, sep='_')
  forArea_con_highF_filename     <- paste(prefix_area,'con_highF',year,scenario, sep='_')
  vol_broadL_highF_filename      <- paste(prefix_vol,'broadL_highF',year,scenario, sep='_')
  vol_broadL_mixedF_filename     <- paste(prefix_vol,'broadL_mixedF',year,scenario, sep='_')
  vol_broadL_copp_filename       <- paste(prefix_vol,'broadL_copp',year,scenario, sep='_')
  vol_con_highF_filename         <- paste(prefix_vol,'con_highF',year,scenario, sep='_')  
  
  forArea_broadL_highF  = raster(forArea_broadL_highF_filename)
  forArea_broadL_mixedF = raster(forArea_broadL_mixedF_filename)
  forArea_broadL_copp   = raster(forArea_broadL_copp_filename)
  forArea_con_highF     = raster(forArea_con_highF_filename)
  vol_broadL_highF      = raster(vol_broadL_highF_filename)
  vol_broadL_mixedF     = raster(vol_broadL_mixedF_filename)
  vol_broadL_copp       = raster(vol_broadL_copp_filename)
  vol_con_highF         = raster(vol_con_highF_filename)
  
  forArea_broadL        = forArea_broadL_highF + forArea_broadL_mixedF + forArea_broadL_copp
  forArea_tot           = forArea_con_highF + forArea_broadL
  vol_tot               = vol_broadL_highF + vol_broadL_mixedF + vol_broadL_copp + vol_con_highF
  forArea_con_pc        = 100 * forArea_con_highF / forArea_tot
  volHa                 = vol_tot*1000000/(forArea_tot/10000) # Vol in Mm^3, Area in m^2 -> vHa in m^3/ha
  forArea_pc_maps[[length(forArea_pc_maps)+1]] <- forArea_con_pc
  volHa_maps[[length(volHa_maps)+1]] <- volHa
}
plot(forArea_pc_maps[[1]], xaxt='n', yaxt='n') # arpege_a1b 2015
plot(forArea_pc_maps[[2]], xaxt='n', yaxt='n') # arpege_a1b 2100
plot(forArea_pc_maps[[3]], xaxt='n', yaxt='n') # arpege_a1b_nomgm 2100
plot(forArea_pc_maps[[4]], xaxt='n', yaxt='n') # arpege_a1b_noexp 2100
plot(forArea_pc_maps[[5]], xaxt='n', yaxt='n') # arpege_a1b_fullexp 2100

plot(volHa_maps[[1]], xaxt='n', yaxt='n') # arpege_a1b 2015
plot(volHa_maps[[2]], xaxt='n', yaxt='n') # arpege_a1b 2100
plot(volHa_maps[[3]], xaxt='n', yaxt='n') # arpege_a1b_nomgm 2100
plot(volHa_maps[[4]], xaxt='n', yaxt='n') # arpege_a1b_noexp 2100
plot(volHa_maps[[5]], xaxt='n', yaxt='n') # arpege_a1b_fullexp 2100

#plot( volHa_maps[[5]], col=rev( rainbow( 99, start=0,end=1 ) ), breaks=seq(min(minValue( volHa_maps[[5]] )),max(maxValue(volHa_maps[[5]])),length.out=100) ) 

#old_a1b_farea     = raster(paste(prefix_area,'con_highF_2100_arpege_a1b_20150512', sep='_'))
#old_fullexp_farea = raster(paste(prefix_area,'con_highF_2100_arpege_a1b_fullExpectations_20150512', sep='_'))
#old_noexp_farea   = raster(paste(prefix_area,'con_highF_2100_arpege_a1b_noExpectations_20150512', sep='_'))

#plot(old_a1b_farea , xaxt='n', yaxt='n') 
#plot(old_fullexp_farea , xaxt='n', yaxt='n') 
#plot(old_noexp_farea , xaxt='n', yaxt='n') 

