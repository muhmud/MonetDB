CREATE TABLE test (pid INTEGER);
INSERT INTO test VALUES (1);
SELECT t.pid FROM test t WHERE FALSE AND (FALSE OR TRUE) AND ( (FALSE AND (FALSE OR FALSE)) OR TRUE );
DROP TABLE test;
