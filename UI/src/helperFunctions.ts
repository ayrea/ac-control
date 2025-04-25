export function getEnumKeys(e: any) {
    const keys = Object.keys(e).map((k) => {
        const value = Number.parseInt(k);
        return Number.isNaN(value) ? k : null;
    })
    .filter(k => k !== null)
    
    return keys;
}

export function getEnumValues(e: any) {
    const values = Object.keys(e).map((k) => {
        const value = Number.parseInt(k);
        return Number.isNaN(value) ? null : value;
    })
    .filter(v => v !== null)
    
    return values;
}