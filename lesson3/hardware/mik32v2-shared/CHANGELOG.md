
# Журнал изменений
Все заметные изменения в проекте будут задокументированы в этом файле.

## Типы изменений
- **Добавлено** — для новых функций.
- **Изменено** — для изменений в существующей функциональности.
- **Устарело** — для функций, которые скоро будут удалены.
- **Удалено** — для удалённых на данный момент.
- **Исправлено** — для любых исправлений багов.
- **Безопасность** — на случай уязвимостей.

## [Невыпущенное] - yyyy-mm-dd
 
В этот раздел следует заносить изменения, которые ещё не были добавлены в новый релиз.

### Добавлено
  
### Изменено
- Изменены названия и маски смещений для Timer16 в соответствии с ТО (TIMER16_ISR и TIMER16_INT).

### Исправлено

## [v0.1.2] - 2024-11-14
 
### Исправлено
- Изменены смещение и маска для калибровочного коэффициента LSI32K

## [v0.1.1] - 2024-10-24
 
### Добавлено
- Добавлены смещения и маски при чтении регистра CHx_CFG в режиме чтения текущего статуса (Current_value=0).
 
### Исправлено
- Исправлена опечатка в названии смещения поля CLEAR_LOCAL_IRQ. 
  
### Удалено
- Удален неиспользуемый скрипт линковки

## [v0.1.0] - 2024-08-15
 
Выпуск набор для сборки приложений под микроконтроллер MIK32V2 с последними изменениями и исправлениями

