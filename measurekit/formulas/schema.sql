CREATE TABLE IF NOT EXISTS formulas (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,            -- 中文名称，如 "一元二次方程求根公式"
    latex TEXT NOT NULL,           -- LaTeX 展示串
    expr TEXT NOT NULL,            -- 内核可求值的表达式，如 "(-b+sqrt(b^2-4*a*c))/(2*a)"
    params TEXT NOT NULL,          -- JSON 数组: [{"symbol":"a","label":"二次项系数","unit":"","domain":"a!=0","default":1}, ...]
    category TEXT NOT NULL,        -- general/probability/statistics/linalg/calculus
    builtin INTEGER NOT NULL DEFAULT 1  -- 1=内置种子 0=用户自定义
);
CREATE INDEX IF NOT EXISTS idx_formulas_category ON formulas(category);
