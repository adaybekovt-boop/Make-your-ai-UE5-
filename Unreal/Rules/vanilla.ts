// The production store has no hooks in its implementation. Use Zustand's own
// vanilla store in a native VM; React and the DOM are not bundled or emulated.
export { createStore as create } from 'zustand/vanilla'
