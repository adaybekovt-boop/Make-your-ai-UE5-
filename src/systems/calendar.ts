import type { GameState } from './types'

export const GAME_START_HOUR = 8
export const GAME_DAY_HOURS = 24
export const GAME_DAY_MINUTES = GAME_DAY_HOURS * 60

type HasElapsedGameHours = Pick<GameState, 'elapsedGameHours'>

export function gameDayFromElapsed(elapsedGameHours: number): number {
  return Math.floor((elapsedGameHours + GAME_START_HOUR) / GAME_DAY_HOURS) + 1
}

export function gameDay(state: HasElapsedGameHours): number {
  return gameDayFromElapsed(state.elapsedGameHours)
}

export function gameClock(elapsedGameHours: number) {
  const totalMinutes = Math.floor((elapsedGameHours + GAME_START_HOUR) * 60)
  const minuteOfDay = ((totalMinutes % GAME_DAY_MINUTES) + GAME_DAY_MINUTES) % GAME_DAY_MINUTES
  const hour = Math.floor(minuteOfDay / 60)
  const minute = minuteOfDay % 60
  return {
    day: Math.floor(totalMinutes / GAME_DAY_MINUTES) + 1,
    hour,
    minute,
    time: `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}`,
  }
}

/** Next midnight in elapsed-game-hour coordinates. */
export function nextGameDayBoundary(elapsedGameHours: number): number {
  return (Math.floor((elapsedGameHours + GAME_START_HOUR) / GAME_DAY_HOURS) + 1) * GAME_DAY_HOURS - GAME_START_HOUR
}
