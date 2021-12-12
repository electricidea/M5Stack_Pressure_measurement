# R Script to analyze and visualize the measured data. 
# 
# see Hackster.io project page for the results:
# https://www.hackster.io/hague/altitude-sickness-due-to-cooker-hoods-721664
DataFilename <- file.choose()
# load the data
Data <- read.table(DataFilename, dec = ".", numerals = c("warn.loss"), header=TRUE, fileEncoding="UTF-8-BOM")
# convert ms to minutes
Data$time <- ((Data$ms-Data$ms[1])/1000)/(60)
# plot the measured raw data
plot(Data$time, Data$Pressure, col="black", type="l", lwd = 1,
     main="Pressure measurements",
     xlab = "time [min]", ylab = "absolute pressure [Pa]")

# look for positions of state changes in the data
last_state = 0
state_cnt = 0
state_change <- cbind(last_state,1)
for (x in 1:length(Data$state)) {
  if(Data$state[x] != last_state){
    state_change <- rbind(state_change, cbind(Data$state[x],x))
    last_state = Data$state[x]
  }
  
}
state_change <- rbind(state_change, cbind(Data$state[length(Data$state)], length(Data$state)))
# calculate the average data for each state period
average_data <- NULL
for (x in 2:length(state_change[,1])) {
  end_x = state_change[x,2]
  start_x = state_change[x-1,2]
  # only use the last 2/3 of the data
  # the first 1/3 is the settling phase
  start_x = start_x + (end_x-start_x)/3
  average_data <- rbind(average_data, cbind(state_change[x-1,1],
                                            mean(Data$time[start_x:end_x]),
                                            mean(Data$Pressure[start_x:end_x]))
                                            )
}
# add the mean values ad red dots to the plot
points(average_data[,2], average_data[,3], type = "p", col = "red", pch=19, cex = 2.0)
# add the fan states as blue line to the plot
par(new = TRUE)
plot(Data$time, Data$state, col="blue", type="l", lwd = 1, lty=2,
     axes = FALSE, bty = "n", xlab = "", ylab = "")

# calculate the pressure differences between the states
delta_ON <- NULL
delta_OFF <- NULL
for (x in 2:length(average_data[,1])) {
  if(average_data[x,1] == 0)
    delta_OFF <- rbind(delta_OFF, average_data[x,3]-average_data[x-1,3])
  if(average_data[x,1] == 1)
    delta_ON <- rbind(delta_ON, average_data[x,3]-average_data[x-1,3])
}

delta_data <- data.frame(A = delta_ON,
                         B = delta_OFF)
# use boxplot to visualize the mean differences
boxplot(delta_data, names=c("OFF -> ON","ON -> OFF"), ylim=c(-3, 3),
        main="Pressure difference",
        xlab = "fan state change", ylab = "pressure difference [Pa]")
abline(h = seq(-3, 3, by = 1), col = "grey", lty = "dotted")
abline(h=0, col="red", lty=2)



