readChannelID = *******;
readAPIKey = '****************';

numMinutes = 120;

[temperature, timestamp] = thingSpeakRead(readChannelID, 'Field', 2, 'NumMinutes', numMinutes, 'ReadKey', readAPIKey);

timestamp(any(isnan(temperature), 2), :) = [];
temperature(any(isnan(temperature), 2), :) = [];

scatter(timestamp, temperature, 10, [0.6350 0.0780 0.1840], 'o', 'filled');
ylabel('Temperature (°C)');
yl = ylim;
ylim([0 (yl(2)+5)]);