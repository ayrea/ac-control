import { ThemeProvider } from '@emotion/react';
import AcControl from './components/AcControl';
import { createTheme } from '@mui/material';
import ApiUrlPrompt from './components/ApiUrlPrompt';
import { useState } from 'react';

const theme = createTheme({
  palette: {
    background: {
      paper: "#ecf5f5"
    }
  }
});

function App() {
  const [baseApiUrl, setBaseApiUrl] = useState<string | null>(localStorage.getItem('baseApiUrl'));

  return (
    baseApiUrl
      ? (<ThemeProvider theme={theme}>< AcControl /></ThemeProvider >)
      : <ApiUrlPrompt setBaseUrl={setBaseApiUrl} />
  )
}

export default App
