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

<img width="1905" height="1801" alt="MarketData_MVP" src="https://github.com/user-attachments/assets/a51cd58f-ff24-4aa2-8f52-c6910cb857b6" />

<img width="1947" height="565" alt="MarketData_Sequences" src="https://github.com/user-attachments/assets/9410ff72-f3d3-4974-9e87-e676d953ca24" />


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
- [x] SMA / EMA
- [x] RSI / MACD
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

<img width="1912" height="980" alt="image" src="https://github.com/user-attachments/assets/ff5d1419-ffdd-4276-a163-087dd8adefa5" />

<img width="1909" height="977" alt="image" src="https://github.com/user-attachments/assets/ddb925e3-1b09-4bc4-b646-d7371da2e7da" />

---

## Статус проекта

Проект находится в активной разработке.  
Архитектура и UML-диаграммы могут изменяться по мере появления новых компонентов.

README обновляется вместе с развитием проекта.

---

## Автор

BortonGo
