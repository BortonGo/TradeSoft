# TradeSoft — Trading Terminal (C++ / Qt)

**TradeSoft** — разрабатываемый торговый терминал на C++ с использованием Qt, предназначенный для анализа рынка, визуализации графиков и дальнейшей интеграции с криптобиржами.

> Статус: **Work in Progress / MVP in Development**

Проект создаётся как pet-project и инженерное портфолио с акцентом на архитектуру, real-time обработку данных, UI/UX и торговую логику.

---

## Цели проекта

- Создать собственный trading-терминал
- Подключение к биржам через API
- Отображение свечных графиков
- Индикаторы и аналитика
- Paper-trading режим
- Управление рисками
- Подготовка к автоторговле

---

## Используемые технологии

- C++17 / C++20
- Qt
- CMake (планируется)
- REST / WebSocket API
- Многопоточность
- Паттерны проектирования
- MVC / MVVM

---

## Architecture & UML (MVP)

На этапе MVP проект строится вокруг модульной архитектуры. UML-диаграммы используются для фиксации текущего дизайна и могут меняться по мере развития системы.

<img width="1112" height="1259" alt="MarketData_MVP" src="https://github.com/user-attachments/assets/21c15074-9582-4a87-b616-7a7fb7be0254" />

<img width="1751" height="823" alt="MarketData_Sequences" src="https://github.com/user-attachments/assets/99420dd7-18b1-477e-b1e3-cd886b6e99c7" />


### High-Level Architecture

- `UI` — Qt интерфейс и виджеты
- `MarketData` — поток котировок и исторические данные
- `Exchange` — клиенты бирж (через общий интерфейс)
- `Network` — REST / WebSocket
- `Strategy` — торговые алгоритмы
- `Risk` — риск-менеджмент
- `Core` — оркестрация и бизнес-логика

Поток данных:

```
Exchange/API
     │
     ▼
MarketData
     │
     ▼
Strategy
     │
     ▼
Risk
     │
     ▼
Core
     │
     ▼
UI
```

---


### Core Classes (план)

- `MarketDataService`
- `IExchangeClient`
- `BingXClient` 
- `ChartWidget`
- `OrderManager`
- `StrategyBase`
- `RiskManager`
- `TradeController`

---

## Планируемая структура проекта

- `core/` — бизнес-логика
- `market/` — рыночные данные
- `exchange/` — клиенты бирж
- `network/` — сетевой слой
- `ui/` — Qt интерфейс
- `charts/` — визуализация
- `strategy/` — стратегии
- `risk/` — риск-менеджмент
- `common/` — утилиты
- `tests/` — тесты
- `docs/uml/` — UML-диаграммы

---

## Roadmap

### Этап 1 — MVP
- [x] Базовая структура проекта
- [x] Главное окно
- [x] Виджет графика
- [x] Таймфреймы
- [x] Выбор инструмента
- [x] Подключение к mock-данным

### Этап 2 — Market Data
- [ ] WebSocket поток
- [ ] Исторические свечи
- [ ] Кэширование

### Этап 3 — Аналитика
- [ ] SMA / EMA
- [ ] RSI / MACD
- [ ] Уровни
- [ ] Объёмы

### Этап 4 — Торговля
- [ ] Paper trading
- [ ] Управление позицией
- [ ] SL / TP
- [ ] Журнал сделок

### Этап 5 — Инфраструктура
- [ ] CMake
- [ ] Логирование
- [ ] Конфигурация
- [ ] Unit-тесты
- [ ] CI/CD

---

## Скриншоты

<img width="1322" height="738" alt="image" src="https://github.com/user-attachments/assets/0ab8ae11-3eb2-47e0-8f98-7ee16931e0a5" />

---

## Статус проекта

Проект находится в активной разработке.  
Архитектура и UML-диаграммы могут изменяться по мере появления новых компонентов.

README обновляется вместе с развитием проекта.

---

## Автор

BortonGo
