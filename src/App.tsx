import { ThemeProvider } from '@emotion/react';
import AcControl from './components/AcControl';
import { createTheme } from '@mui/material';

const theme = createTheme({
  palette: {
    background: {
      paper: "#ecf5f5"
    }
  }
});

function App() {
  return (
    <ThemeProvider theme={theme} >
      <AcControl />
    </ThemeProvider>
  )
}

export default App
