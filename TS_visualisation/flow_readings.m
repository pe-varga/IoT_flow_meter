readChannelID = *******;
readAPIKey = '***************';

numMinutes = 120;

[mode, timestamp] = thingSpeakRead(readChannelID, 'Field', 1, 'NumMinutes', numMinutes, 'ReadKey', readAPIKey);
[flows] = thingSpeakRead(readChannelID, 'Field', [3:7], 'NumMinutes', numMinutes, 'ReadKey', readAPIKey);

flows(any(isnan(mode), 2), :) = [];
timestamp(any(isnan(mode), 2), :) = [];
mode(any(isnan(mode), 2), :) = [];

for i = 1:size(timestamp, 1)
    if mode(i) == 1
        min = 1;
    elseif mode(i) == 2
        min = 5;
    else
        min = 10;
    end
    for j = 1:5
        time((i-1)*5 + j) = timestamp(i) - minutes((5-(j-1)) * min);
        data((i-1)*5 + j) = flows(i, j);
    end
end

time = reshape(time, [size(time, 2), 1]);
data = reshape(data, [size(data, 2), 1]);

stairs(time, data, 'LineWidth', 2, 'Color', '#4DBEEE');
ylabel('Flow (mm/h)')