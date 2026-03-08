workspace "Вариант 12 — Бюджетирование" "C4-модель системы бюджетирования (Structurizr DSL)" {
  !identifiers hierarchical

  model {
    user = person "Пользователь" "Планирует доходы/расходы и анализирует динамику бюджета."

    emailSystem = softwareSystem "Email-сервис" "Отправка писем подтверждения регистрации." {
      tags "External"
    }

    budgeting = softwareSystem "Система бюджетирования" "Планирование доходов/расходов и расчёт динамики бюджета за период." {
      spa = container "Web Application" "Пользовательский интерфейс для планирования бюджета." "React (SPA)" {
        tags "WebApp"
      }

      apiGateway = container "API Gateway" "Единая точка входа: маршрутизация запросов, проверка JWT, rate limiting." "Kong" {
        tags "Gateway"
      }

      userService = container "User Service" "Регистрация, авторизация (JWT), поиск пользователей." "C++ / userver (REST)"
      budgetService = container "Budget Service" "Управление планируемыми доходами и расходами, расчёт динамики бюджета за период." "C++ / userver (REST)"

      db = container "Database" "Пользователи, планируемые доходы и расходы." "PostgreSQL" {
        tags "Database"
      }

      cache = container "Cache" "Кэш результатов расчёта динамики бюджета (cache-aside, TTL)." "Redis" {
        tags "Cache"
      }
    }

    user -> budgeting "Планирует бюджет и анализирует динамику" "HTTPS"
    budgeting -> emailSystem "Отправляет уведомления" "SMTP"

    user -> budgeting.spa "Планирует бюджет, просматривает аналитику" "HTTPS"
    budgeting.spa -> budgeting.apiGateway "Выполняет API-запросы" "HTTPS/REST"
    budgeting.apiGateway -> budgeting.userService "Маршрутизирует запросы /users" "HTTP/REST"
    budgeting.apiGateway -> budgeting.budgetService "Маршрутизирует запросы /budget" "HTTP/REST"
    budgeting.userService -> budgeting.db "Читает и записывает данные пользователей" "SQL"
    budgeting.userService -> emailSystem "Отправляет письмо подтверждения" "SMTP"
    budgeting.budgetService -> budgeting.db "Читает и записывает доходы/расходы, агрегирует данные" "SQL"
    budgeting.budgetService -> budgeting.cache "Кэширует результаты расчёта динамики" "RESP"
  }

  views {
    systemContext budgeting "C1-SystemContext" "Система бюджетирования в контексте пользователей и внешних систем" {
      include *
      autolayout lr
    }

    container budgeting "C2-Containers" "Внутренняя структура системы бюджетирования" {
      include *
      autolayout lr
    }

    dynamic budgeting "Dynamic-BudgetDynamics" "Посчитать динамику бюджета за период" {
      autolayout lr

      user -> budgeting.spa "Запрашивает динамику бюджета за период"
      budgeting.spa -> budgeting.apiGateway "GET /budget/dynamics?from&to (Bearer JWT)"
      budgeting.apiGateway -> budgeting.budgetService "Перенаправляет авторизованный запрос"
      budgeting.budgetService -> budgeting.cache "Проверка кэша: dynamics:{userId,from,to}"
      budgeting.budgetService -> budgeting.db "Cache miss — SELECT доходов и расходов за период"
      budgeting.budgetService -> budgeting.cache "SET dynamics:{userId,from,to} с TTL"
    }

    styles {
      element "Person" {
        shape person
        background #08427b
        color #ffffff
      }

      element "Software System" {
        background #1168bd
        color #ffffff
      }

      element "External" {
        background #999999
        color #ffffff
      }

      element "Container" {
        background #438dd5
        color #ffffff
      }

      element "WebApp" {
        shape webbrowser
      }

      element "Gateway" {
        shape hexagon
      }

      element "Database" {
        shape cylinder
      }

      element "Cache" {
        shape roundedbox
      }
    }
  }
}
