SELECT INTERVAL '1' HOUR / 2, INTERVAL '1' HOUR / 2.0, INTERVAL '1' HOUR * 1000 / 2000, INTERVAL '1' HOUR * 1000.0 / 2000.0; --all output 1800.000
SELECT INTERVAL '1' HOUR * 1000 / 1800000; -- 2.000
SELECT INTERVAL '1' HOUR * CAST(1000 AS DOUBLE);
SELECT INTERVAL '1' MONTH * 1.2, INTERVAL '1' SECOND * 1.2;
SELECT INTERVAL '1' HOUR / INTERVAL '1800' SECOND; --error on typing branch, cannot divide intervals
