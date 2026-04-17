import { Fan } from 'lucide-react'

import { ToggleGroup, ToggleGroupItem } from '@/components/ui/toggle-group'
import type { FanSpeed } from '@/types/ac'
import { cn } from '@/lib/utils'

const SPEEDS: { value: FanSpeed; label: string; iconClass: string }[] = [
  { value: 'low', label: 'Low', iconClass: 'size-4 opacity-70' },
  { value: 'medium', label: 'Med', iconClass: 'size-5' },
  { value: 'high', label: 'High', iconClass: 'size-6' },
]

type FanSpeedSelectorProps = {
  fanSpeed: FanSpeed
  power: boolean
  onFanSpeedChange: (speed: FanSpeed) => void
}

export function FanSpeedSelector({
  fanSpeed,
  power,
  onFanSpeedChange,
}: FanSpeedSelectorProps) {
  return (
    <div className="space-y-2">
      <p className="text-muted-foreground px-1 text-xs font-medium tracking-wide uppercase">
        Fan speed
      </p>
      <ToggleGroup
        type="single"
        value={fanSpeed}
        onValueChange={(v) => {
          if (v) onFanSpeedChange(v as FanSpeed)
        }}
        variant="outline"
        spacing={0}
        className={'grid w-full grid-cols-3 gap-0 rounded-lg'}
      >
        {SPEEDS.map(({ value, label, iconClass }) => (
          <ToggleGroupItem
            key={value}
            value={value}
            className={cn(
              'flex min-h-11 flex-1 flex-col gap-1 px-2 py-6',
              power
                ? 'data-[state=on]:bg-primary data-[state=on]:text-primary-foreground'
                : 'data-[state=on]:bg-muted data-[state=on]:text-muted-foreground',
            )}
            aria-label={`Fan speed ${label}`}
          >
            <Fan className={cn('mx-auto shrink-0', iconClass)} aria-hidden />
            <span className="text-xs font-medium">{label}</span>
          </ToggleGroupItem>
        ))}
      </ToggleGroup>
    </div>
  )
}
