import { Switch } from '@/components/ui/switch'
import type { Zone } from '@/types/ac'

type ZoneControlProps = {
  zones: Zone[]
  onToggleZone: (zoneId: number) => void
}

export function ZoneControl({ zones, onToggleZone }: ZoneControlProps) {
  return (
    <div className="space-y-2">
      <p className="text-muted-foreground px-1 text-xs font-medium tracking-wide uppercase">
        Zones
      </p>
      {zones.length === 0 && (
        <p className="text-muted-foreground px-1 text-sm">
          Waiting for zone information...
        </p>
      )}
      <div className="grid grid-cols-2 gap-3">
        {zones.map((zone) => (
          <div
            key={zone.id}
            className={'flex min-h-11 items-center justify-between gap-2 rounded-lg border bg-muted/40 px-3 py-2'}
          >
            <label
              htmlFor={`zone-${zone.id}`}
              className="text-sm font-medium leading-none peer-disabled:cursor-not-allowed peer-disabled:opacity-70"
            >
              {zone.name}
            </label>
            <Switch
              id={`zone-${zone.id}`}
              checked={zone.enabled}
              onCheckedChange={() => onToggleZone(zone.id)}
              aria-label={`${zone.name} ${zone.enabled ? 'on' : 'off'}`}
            />
          </div>
        ))}
      </div>
    </div>
  )
}
