data = dlmread("../../res/2017-04-11_-_14-06-06_4_raw.csv", ";");
data = data(:, 1:2);
noise = dlmread("../../res/noise.csv")(:, 1:2);
figure(1);
plot(data);
grid on;
title("Сырые данные");
saveas(1, "1.png");
# 1. Вычесть шумовую дорожку из сигналов (содержится в файле noise.csv)
data -= noise;
figure(2);
plot(data);
grid on;
title("После удаления шума");
saveas(2, "2.png");
# 2. Удалить «нулевую» дорожку до оцифрованных значений отражённого сигнала
low_border = 100;
idx_to_rem = [];	
for i = [1:length(data)]
	if abs(data(i, :)) > low_border
		break;
	else
		idx_to_rem = [idx_to_rem i];	
	endif;		
endfor;
data(idx_to_rem, :) = [];	
figure(3);
plot(data);
grid on;
title("После удаления нулевой дорожки");
saveas(3, "3.png");
# 3. Инвертировать сигнал
data = -data;
figure(4);
plot(data);
grid on;
title("После инвертирования сигнала");
saveas(4, "4.png");

# 4. Привести нормировку, чтобы значения сигнала были в диапазоне от [0; signal_max]
max_signal = abs(max(data));
max_data = max(data);
min_data = min(data);	
data -= min(data);

figure(5);
plot(data);
grid on;
title("После нормирования сигнала");
saveas(5, "5.png")

# 5. Нормировать сигнал по расстоянию на 12 км
x = linspace(0, length(data));
t = linspace(0, 12000, length(data));		
data = (data - min(x)) ./ (max(x) - min(x)) .* (12000 - 0) + 0;
	
figure(6);
plot(data);
grid on;
title("После нормирования сигнала по расстоянию");
saveas(6, "6.png")

# 6. Применить фильтрацию, например, медианным фильтром
window_size = 3;
filtered = [];
cor_data = [data(1, :);	 data;	 data(end, :)];	
for i = [2:length(cor_data) - 1]
	window = sort([cor_data(i - 1, :); cor_data(i, :); cor_data(i + 1, :)]);
	filtered = [filtered, window(2)];	
endfor;

figure(7);
plot(data);
grid on;
title("После применения медианного фильтра");
saveas(7, "7.png")

# 7. Результирующий сигнал должен быть сформирован из двух предобработанных ранее (см. рис. 3), согласно формуле:
parallel = data(:, 1);
perpendicular = data(:, 2);	
result_data = sqrt(parallel .^ 2 + perpendicular .^ 2);
figure(8);	
plot(t, result_data);
grid on;
title("Результирующий сигнал");
saveas(8, "8.png")
