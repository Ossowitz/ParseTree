# Дерево синтаксического разбора

## Деревья выражений

### Подход к решению

Первый шаг при построении дерева синтаксического разбора - это разбить строку на список токенов. Их насчитывается
**четыре** вида: левая скобка, правая скобка, оператор и операнд. Мы знаем, что прочитанная левая скобка всегда означает
начало нового выражения и, следовательно, необходимо создать связанное с ним новое дерево. И наоборот, когда мы
считываем правую  скобку, мы завершаем выражение. Также известно, что операнды будут листьями и потомками своих операторов. 
Наконец, мы знаем, что каждый оператор имеет как правого, так и левого потомков.

### Структуры данных и алгоритмы

В данном проекте использовано бинарное дерево, которое реализуется с помощью двух указателей на потомков: левого и
правого. Оно также содержит символьную переменную *char op* для сохранения операций вычисления или буквенных констант.
Кроме этого, дерево содержит поле *int value*, которое отвечает за значение листа, если оно должно содержать численное
значение.

### Алгоритм построения дерева разбора для математического выражения

1. Программа считывает строку и разбивает её на токены. _Токены - это элементы математического выражения_. Это могут
   быть
   круглые скобки, знаки операций, числа или буквы
2. Создаётся дерево выражения с помощью этих токенов.
    + В префиксной форме дерево строится рекурсивно, слева считывается знак операции, затем разбивается выражение в
      скобках запятой на два поддерева и на эти поддеревья применяется эта же функция
    + В постфиксной форме действия аналогичные кроме того, что токены считываются справа налево
    + Для естественного языка токены считываются справа налево, затем ищется самая неприоритетная операция, с помощью
      неё выражение разделяется на два поддерева, и на эти поддеревья применяется эта же операция.
3. Вывод выражения в обеих формах работает рекурсивно. Отличие заключается в постановке символа операции.
В префиксной форме сначала пишется знак операции, затем выводятся левое и право поддеревья. В постфиксной форме сначала 
выводятся левое и правое поддеревья, а затем пишется знак операции.
4. В постфиксной форме сначала происходит процедура замены буквенной константы на численное значение, а затем идёт рекурсивной вычисление функции.
