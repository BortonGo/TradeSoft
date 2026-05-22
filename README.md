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

### Этап 1 — Основа приложения
- [x] Базовая структура проекта
- [x] Главное окно
- [x] Виджет графика
- [x] Таймфреймы
- [x] Выбор инструмента
- [x] Подключение источника данных
- [x] Базовая доменная модель
- [x] Разделение на UI / core / service / exchange / domain

### Этап 2 — Market Data
- [x] Загрузка исторических свечей
- [x] Подключение биржевого клиента
- [x] Обновление текущей свечи в realtime
- [ ] WebSocket поток
- [x] Кэширование / локальное хранение данных
- [ ] Повышение устойчивости data-layer

### Этап 3 — Аналитика и визуализация
- [x] EMA
- [x] RSI
- [x] ATR
- [x] Donchian / ценовые каналы
- [x] Отображение индикаторов на графике
- [x] Визуализация торговых уровней
- [ ] SMA
- [ ] MACD
- [ ] Уровни в расширенном виде
- [ ] Объёмы

### Этап 4 — Торговый контур
- [x] Базовая архитектура торгового контура
- [x] Demo / paper execution foundation
- [x] Базовое управление позицией
- [x] SL / TP логика
- [x] Журнал сделок
- [x] Риск-менеджмент
- [ ] Полноценный paper trading сценарий
- [ ] Расширенное управление позицией
- [ ] Полировка торгового UX

### Этап 5 — Стратегии и backtesting
- [x] Базовая архитектура стратегий
- [x] Strategy runner / controller
- [x] Первые стратегии
- [x] Каркас backtest-модуля
- [ ] Полноценный backtest engine
- [ ] Метрики и отчёты
- [ ] Проверка стратегий на истории

### Этап 6 — Инфраструктура
- [x] CMake
- [x] Логирование
- [x] Конфигурация
- [x] Unit-тесты
- [x] CI/CD

---

## Сборка на macOS

Установи Qt и CMake, затем собери проект из отдельной build-директории:

```bash
brew install cmake qt
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
cmake --build build
open build/TradeSoft.app
```

Если Qt установлен не через Homebrew, передай путь к Qt в `CMAKE_PREFIX_PATH`, например `/Users/you/Qt/6.7.0/macos`.

### Тесты

Тесты включены в стандартную CMake-сборку:

```bash
cmake --build build --target TradeSoftTests
ctest --test-dir build --output-on-failure
```

### Логи

При запуске приложение пишет Qt-логи в пользовательскую папку приложения:

- macOS: `~/Library/Application Support/BortonGo/TradeSoft/tradesoft.log`

### Конфигурация

При первом запуске создаётся JSON-конфиг:

- macOS: `~/Library/Application Support/BortonGo/TradeSoft/config.json`

В нём можно менять дефолтный инструмент, таймфрейм, интервал realtime polling и список символов.

### Кэш свечей

Свечи для графика кэшируются локально, чтобы ускорить повторные запуски и иметь fallback при сетевой ошибке:

- macOS: `~/Library/Caches/BortonGo/TradeSoft/marketdata`

---

## Скриншоты

<img width="1909" height="978" alt="image" src="https://github.com/user-attachments/assets/fc4b5ac7-dc64-4def-9559-f379c5f0ffcb" />

<img width="1908" height="980" alt="image" src="https://github.com/user-attachments/assets/b79d4d67-9f8b-40db-828b-cfbf8ca1526e" />

<img width="1910" height="975" alt="image" src="https://github.com/user-attachments/assets/1c5c1ec5-845c-4345-8d4e-497318033a17" />

---

## Статус проекта

Проект находится в активной разработке.  
Архитектура и UML-диаграммы могут изменяться по мере появления новых компонентов.

README обновляется вместе с развитием проекта.

---

## Автор

BortonGo
