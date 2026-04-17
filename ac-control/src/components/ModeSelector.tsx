import {
  Droplets,
  Flame,
  Gauge,
  Snowflake,
  Wind,
} from 'lucide-react'

import { ToggleGroup, ToggleGroupItem } from '@/components/ui/toggle-group'
import { cn } from '@/lib/utils'
import type { ACMode } from '@/types/ac'

const MODES: { value: ACMode; label: string; Icon: typeof Snowflake }[] = [
  { value: 'cool', label: 'Cool', Icon: Snowflake },
  { value: 'heat', label: 'Heat', Icon: Flame },
  { value: 'vent', label: 'Vent', Icon: Wind },
  { value: 'dry', label: 'Dry', Icon: Droplets },
  { value: 'auto', label: 'Auto', Icon: Gauge },
]

type ModeSelectorProps = {
  mode: ACMode
  power: boolean
  onModeChange: (mode: ACMode) => void
}

export function ModeSelector({ mode, power, onModeChange }: ModeSelectorProps) {
  return (
    <div className="space-y-2">
      <p className="text-muted-foreground px-1 text-xs font-medium tracking-wide uppercase">
        Mode
      </p>
      <ToggleGroup
        type="single"
        value={mode}
        onValueChange={(v) => {
          if (v) onModeChange(v as ACMode)
        }}
        variant="outline"
        spacing={0}
        className={'grid w-full grid-cols-5 gap-0 rounded-lg'}
      >
        {MODES.map(({ value, label, Icon }) => (
          <ToggleGroupItem
            key={value}
            value={value}
            className={cn(
              'flex min-h-11 flex-1 flex-col gap-0.5 px-1 py-2',
              power
                ? 'data-[state=on]:bg-primary data-[state=on]:text-primary-foreground'
                : 'data-[state=on]:bg-muted data-[state=on]:text-muted-foreground',
            )}
            aria-label={label}
          >
            <Icon className="mx-auto size-5 shrink-0" aria-hidden />
            <span className="text-[10px] font-medium leading-none">{label}</span>
          </ToggleGroupItem>
        ))}
      </ToggleGroup>
    </div>
  )
}
