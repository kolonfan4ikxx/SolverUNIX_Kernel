# SolverUNIX Kernel|SolverUNIX/KNG UNIX
An official SolverUNIX Kernel repository

**Ядро операционной системы SolverUNIX KNG**

**The kernel of SolverUNIX KNG operating system**

![Architecture](https://img.shields.io/badge/Architecture-x86__64-blue)
![Language](https://img.shields.io/badge/Language-C%20%2F%20Assembly-orange)
![Status](https://img.shields.io/badge/Status-Experimental-red)

---

# 🇷🇺 Русский

## О проекте

**SolverUNIX** — независимая UNIX-подобная операционная система, разрабатываемая в рамках проекта SolverUNIX KNG.

Главная цель проекта — создание собственной операционной системы с независимым ядром, системными библиотеками, утилитами, системой инициализации и инфраструктурой управления пакетами.

SolverUNIX не является дистрибутивом Linux и не использует ядро Linux.

Проект стремится создать собственную программную экосистему с нуля.

## Цели проекта

- Разработка собственного ядра для архитектуры x86_64.
- Создание `solibc` — собственной реализации стандартной библиотеки C.
- Разработка `sol6` — собственной системы инициализации.
- Реализация управления памятью и процессами.
- Разработка собственных системных вызовов.
- Создание модульной архитектуры драйверов.
- Разработка собственной пользовательской среды.
- Поддержка загрузки ELF-программ.

## Архитектура

| Компонент | Описание |
|---|---|
| `SolverUNIX Kernel` | Основное ядро ОС |
| `solibc` | Собственная библиотека C |
| `sol6` | Система инициализации |
| `solas` | Интерфейс системных команд |
| `solpkg` | Система управления пакетами |
| `KFS` | Файловая инфраструктура SolverUNIX |

Некоторые компоненты находятся в планах разработки и могут быть ещё не реализованы.

## Целевая платформа

| Свойство | Значение |
|---|---|
| Архитектура | x86_64 |
| Платформа | Bare metal |
| Формат исполняемых файлов | ELF |
| Целевой компилятор | `x86_64-elf` |
| Система сборки | GNU Make |
| Языки | C, Assembly |

## Разработка

Ядро SolverUNIX находится на экспериментальной стадии активной разработки.

Основной принцип разработки — независимость от существующих ядер операционных систем.

### Необходимые инструменты

- GNU Make
- `x86_64-elf-gcc`
- `x86_64-elf-binutils`
- ELF-совместимый линковщик
- QEMU для виртуального тестирования

**Важно:** MinGW GCC (`x86_64-w64-mingw32`) предназначен для создания Windows-программ и не заменяет кросс-компилятор `x86_64-elf`.

### Сборка

Инструкции по сборке будут добавлены после стабилизации системы сборки ядра.

## Структура проекта

Структура каталогов может изменяться в процессе разработки.

```text
SolverUNIX/
├── kernel/
├── include/
├── linker.ld
├── Makefile
└── README.md
```

## Дорожная карта

- [ ] Настройка загрузки x86_64
- [ ] Точка входа ядра
- [ ] Базовый вывод в терминал
- [ ] GDT и IDT
- [ ] Обработка прерываний
- [ ] Управление физической памятью
- [ ] Управление виртуальной памятью
- [ ] Базовый планировщик процессов
- [ ] Интерфейс системных вызовов
- [ ] ELF-загрузчик
- [ ] Инфраструктура драйверов
- [ ] Интеграция с solibc
- [ ] Интеграция с sol6

## Философия проекта

SolverUNIX строится вокруг одной основной идеи:

**Независимость.**

Проект направлен на создание собственного ядра и собственной системной инфраструктуры вместо использования готового ядра другой операционной системы.

## Лицензия

Информация о лицензии будет добавлена позднее.

---

# 🇬🇧 English

## About

**SolverUNIX** is an independent UNIX-like operating system developed as part of the SolverUNIX KNG project.

The main goal is to build an operating system with its own independent kernel, system libraries, utilities, initialization system and package management infrastructure.

SolverUNIX is not a Linux distribution and does not use the Linux kernel.

The project aims to build its own software ecosystem from the ground up.

## Project goals

- Develop an independent x86_64 kernel.
- Create `solibc` — a native C standard library implementation.
- Develop `sol6` — a native initialization system.
