INSERT INTO users (login, password, first_name, last_name) VALUES
    ('alexeev',   crypt('pass123',  gen_salt('bf')), 'Алексей',  'Иванов'),
    ('mpetrova',  crypt('secret42', gen_salt('bf')), 'Мария',    'Петрова'),
    ('dsidorov',  crypt('qwerty',   gen_salt('bf')), 'Дмитрий',  'Сидоров'),
    ('akozlova',  crypt('p@ssw0rd', gen_salt('bf')), 'Анна',     'Козлова'),
    ('snovikov',  crypt('nov1kov',  gen_salt('bf')), 'Сергей',   'Новиков'),
    ('emorozova', crypt('frost99',  gen_salt('bf')), 'Елена',    'Морозова'),
    ('avolkov',   crypt('wolf777',  gen_salt('bf')), 'Андрей',   'Волков'),
    ('olebedeva', crypt('swan2026', gen_salt('bf')), 'Ольга',    'Лебедева'),
    ('mkuznetsov',crypt('smith00',  gen_salt('bf')), 'Михаил',   'Кузнецов'),
    ('tsokolova', crypt('falc0n',   gen_salt('bf')), 'Татьяна',  'Соколова'),
    ('npopov',    crypt('pop1234',  gen_salt('bf')), 'Николай',  'Попов'),
    ('nfedorova', crypt('fed0r0va', gen_salt('bf')), 'Наталья',  'Федорова');
