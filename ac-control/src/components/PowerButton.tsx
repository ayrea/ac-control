import { Power } from 'lucide-react'

import { Button } from '@/components/ui/button'
import { cn } from '@/lib/utils'

type PowerButtonProps = {
  power: boolean
  onToggle: () => void
}

export function PowerButton({ power, onToggle }: PowerButtonProps) {
  return (
    <Button
      type="button"
      variant={power ? 'default' : 'outline'}
      size="icon-lg"
      className={cn(
        'min-h-12 min-w-12 shrink-0 rounded-full',
        power &&
          'bg-primary text-primary-foreground shadow-md ring-2 ring-emerald-300 ring-offset-2 ring-offset-background',
      )}
      onClick={onToggle}
      aria-pressed={power}
      aria-label={power ? 'Turn air conditioning off' : 'Turn air conditioning on'}
    >
      <Power className="size-6" aria-hidden />
    </Button>
  )
}
