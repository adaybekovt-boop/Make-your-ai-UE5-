/** A codec import must never open IndexedDB in a native process. */
export function openDB(): never { throw new Error('Native storage must use the validated host save adapter') }
