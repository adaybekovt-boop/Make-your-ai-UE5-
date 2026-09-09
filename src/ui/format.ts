export { money, signedMoney } from '../shared/currency'
export { gameClock } from '../systems/calendar'

const number = new Intl.NumberFormat('ru-RU', { maximumFractionDigits: 0 })
const decimal = new Intl.NumberFormat('ru-RU', { maximumFractionDigits: 1 })

export const quantity = (value: number) => decimal.format(value)
export const percent = (value: number) => `${number.format(value * 100)}%`
export const hours = (value: number | null) => value === null ? 'не окупится' : `${quantity(value)} игр. ч`

export function serversText(count: number) {
  const lastTwo = count % 100
  const ending = lastTwo >= 11 && lastTwo <= 14 ? 'серверов' : count % 10 === 1 ? 'сервер' : count % 10 >= 2 && count % 10 <= 4 ? 'сервера' : 'серверов'
  return `${count} ${ending}`
}
