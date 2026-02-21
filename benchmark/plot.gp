# Настройки терминала (вывод в PNG изображение высокого качества)
set terminal pdfcairo size 10,7 enhanced font 'Arial,16'
set output 'benchmark_graph.pdf'

# Настройки заголовков и осей
set title "Зависимость времени отрисовки кадра от количества шестеренок" font ",22"
set xlabel "Количество шестеренок (шт.)" font ",20"
set ylabel "Среднее время кадра (мс)" font ",20"

# Настройка сетки
set grid xtics ytics mytics
set style line 101 lc rgb '#808080' lt 1 lw 1
set border 3 back ls 101

# Настройка стиля линии
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 ps 1.5   # Синяя линия с точками
set style fill solid 1.0 border -1

# Настройка легенды (ключа)
set key left top box opaque

# Настройка формата входных данных (CSV)
set datafile separator ";"

# Пропуск первой строки (заголовка CSV)
# Используем 'every ::1' (начиная со 2-й строки, индекс 1)

# Построение графика
# $1 - первый столбец (GearsCount)
# $2 - второй столбец (AvgFrameTime_ms)

plot 'benchmark_results.csv' every ::1 using 1:2 with linespoints ls 1 title 'Среднее время кадра'
